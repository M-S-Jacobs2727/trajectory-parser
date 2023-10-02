from pathlib import Path

import numpy as np

import mdtrajectory.mdtrajectory as dp

class TestGenerators:
    dumpdir = Path("test/test_dumps")
    
    def test_bin_file(self):
        for i, frame in enumerate(dp.dumpfile(self.dumpdir / "dump.bin", binary=True)):
            assert frame.timestep == i * 50
            assert frame.natoms == 4000
            assert np.allclose(frame.tilt, np.zeros(3))
            assert np.allclose(frame.box[::2], np.zeros(3))
            assert np.allclose(frame.box[1::2], np.zeros(3) + 16.795961913825074)
            assert frame.columns == ['id', 'type', 'x', 'y', 'z', 'xu', 'yu', 'zu', 'vx', 'vy', 'vz']
            assert frame.data.shape == (4000, 11)

    def test_bin_files(self):
        dumplist = list(self.dumpdir.glob("dump.*.bin"))
        dumplist.sort(key=lambda path: int(path.name[5:-4]))
        for i, frame in enumerate(dp.dumpfiles(dumplist, binary=True)):
            assert frame.timestep == i * 50
            assert frame.natoms == 4000
            assert np.allclose(frame.tilt, np.zeros(3))
            assert np.allclose(frame.box[::2], np.zeros(3))
            assert np.allclose(frame.box[1::2], np.zeros(3) + 16.795961913825074)
            assert frame.columns == ['id', 'type', 'x', 'y', 'z', 'xu', 'yu', 'zu', 'vx', 'vy', 'vz']
            assert frame.data.shape == (4000, 11)
    
    def test_txt_file(self):
        for i, frame in enumerate(dp.dumpfile(self.dumpdir / "dump.txt")):
            assert frame.timestep == i * 50
            assert frame.natoms == 4000
            assert np.allclose(frame.tilt, np.zeros(3))
            assert np.allclose(frame.box[::2], np.zeros(3))
            assert np.allclose(frame.box[1::2], np.zeros(3) + 16.795961913825074)
            assert frame.columns == ['id', 'type', 'x', 'y', 'z', 'xu', 'yu', 'zu', 'vx', 'vy', 'vz']
            assert frame.data.shape == (4000, 11)

    def test_txt_files(self):
        dumplist = list(self.dumpdir.glob("dump.*.txt"))
        dumplist.sort(key=lambda path: int(path.name[5:-4]))
        for i, frame in enumerate(dp.dumpfiles(dumplist)):
            assert frame.timestep == i * 50
            assert frame.natoms == 4000
            assert np.allclose(frame.tilt, np.zeros(3))
            assert np.allclose(frame.box[::2], np.zeros(3))
            assert np.allclose(frame.box[1::2], np.zeros(3) + 16.795961913825074)
            assert frame.columns == ['id', 'type', 'x', 'y', 'z', 'xu', 'yu', 'zu', 'vx', 'vy', 'vz']
            assert frame.data.shape == (4000, 11)

    def test_equal(self):
        bin_dumplist = list(self.dumpdir.glob("dump.*.bin"))
        txt_dumplist = list(self.dumpdir.glob("dump.*.txt"))
        bin_dumplist.sort(key=lambda path: int(path.name[5:-4]))
        txt_dumplist.sort(key=lambda path: int(path.name[5:-4]))

        for frame1, frame2, frame3, frame4 in zip(
            dp.dumpfile(self.dumpdir / "dump.bin", binary=True),
            dp.dumpfiles(bin_dumplist, binary=True),
            dp.dumpfile(self.dumpdir / "dump.txt"),
            dp.dumpfiles(txt_dumplist),
        ):
            assert frame1 == frame2 == frame3 == frame4
            assert np.allclose(frame1.data, frame2.data)
            assert np.allclose(frame1.data, frame3.data)
            assert np.allclose(frame1.data, frame4.data)