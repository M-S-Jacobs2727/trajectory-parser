import io
from pathlib import Path
from struct import unpack

from attrs import frozen, field
import numpy as np
import numpy.typing as npt


intArr = npt.NDArray[np.int64]
dataArr = npt.NDArray[np.float64]

@frozen(kw_only=True)
class Frame:
    timestep: int
    natoms: int
    box: dataArr = field(eq=False)
    tilt: dataArr = field(eq=False)

    data: dataArr = field(eq=False)
    columns: list[str]

    def col(self, *args: str) -> npt.NDArray:
        if len(self.columns) == 0:
            raise RuntimeError("No column names found")
        if len(args) == 0:
            raise RuntimeError("No column names given")
        
        if len(args) == 1:
            idx = self.columns.index(args[0])
            outdata = self.data[:, idx]
            return outdata
        
        outdata = np.zeros((self.natoms, len(args)))
        for i, arg in enumerate(args):
            idx = self.columns.index(arg)
            outdata[:, i] = self.data[:, idx]
        
        return outdata

def readTxtFrame(stream: io.TextIOBase):
    line = stream.readline().strip()
    if line == "":
        return
    while True:
        keyword = line[6:]
        if keyword in ["UNITS", "TIME"]:
            stream.readline()
        elif keyword == "TIMESTEP":
            timestep = int(stream.readline().strip())
        elif keyword == "NUMBER OF ATOMS":
            natoms = int(stream.readline().strip())
        elif keyword[:3] == "BOX":
            tri = len(keyword) > 20
            box = np.zeros(6, dtype=np.float64)
            tilt = np.zeros(3, dtype=np.float64)
                
            for i in range(3):
                vals = stream.readline().split()
                box[2*i] = float(vals[0])
                box[2*i+1] = float(vals[1])
                if tri:
                    tilt[i] = float(vals[2])
        elif keyword[:5] == "ATOMS":
            break
        else:
            raise SyntaxError(f"2, {line}")
                
        line = stream.readline().strip()
        if line[:5] != "ITEM:":
            raise SyntaxError(f"1, {line}")

    columns = line[12:].split()
    
    data = np.loadtxt(stream, max_rows=natoms, dtype=np.float64)
    
    return Frame(
        timestep=timestep,
        natoms=natoms,
        box=box,
        tilt=tilt,
        data=data,
        columns=columns,
    )

def readBinFrame(stream: io.BytesIO):
    b = stream.read(8)
    if len(b) < 8:
        return
    
    timestep, = unpack("=q", b)
    revision = 0
    if timestep < 0:
        magic_str_len = -timestep
        magic_str = stream.read(magic_str_len).decode()
        endian, revision, timestep = unpack("=llq", stream.read(16))
    
    box = np.zeros(6, dtype=np.float64)
    natoms, triclinic, *args = unpack("=q7l6d", stream.read(84))
    boundary_settings = args[:6]
    box[:] = args[6:]

    tilt = np.zeros(3, dtype=np.float64)
    if triclinic:
        tilt[:] = unpack("=3d", stream.read(24))

    ncols, = unpack("=l", stream.read(4))

    columns = list()
    if revision > 1:
        n, = unpack("=l", stream.read(4))
        if n:
            units_string = stream.read(n).decode()

        flag, = unpack("=?", stream.read(1))
        if flag:
            time, = unpack("=d", stream.read(8))
        
        n, = unpack("=l", stream.read(4))
        columns = stream.read(n).decode().split()
    
    bin_nchunk, = unpack("=l", stream.read(4))

    data = np.zeros((natoms, ncols), dtype=np.float64)
    idx = 0
    for _ in range(bin_nchunk):
        n, = unpack("=l", stream.read(4))
        nrows = n // ncols
        data[idx:idx+nrows] = np.asarray(unpack(f"={n}d", stream.read(n*8))).reshape(nrows, ncols)
        idx += nrows
    
    return Frame(
        timestep=timestep,
        natoms=natoms,
        box=box,
        tilt=tilt,
        data=data,
        columns=columns,
    )

def dumpfiles(filepaths: list[Path], binary: bool = False):
    if binary:
        readFrame = readBinFrame
        mode = "rb"
    else:
        readFrame = readTxtFrame
        mode = "r"

    for filepath in filepaths:
        if not filepath.is_file():
            raise FileNotFoundError(f"{filepath}")
    
        with open(filepath, mode) as f:
            frame = readFrame(f)

        yield frame

def dumpfile(filepath: Path, binary: bool = False):
    if not filepath.is_file():
        raise FileNotFoundError(f"Could not locate file {filepath}")
    
    if binary:
        readFrame = readBinFrame
        mode = "rb"
    else:
        readFrame = readTxtFrame
        mode = "r"

    with open(filepath, mode) as f:
        while True:
            frame = readFrame(f)
            if frame is None:
                return
            yield frame
