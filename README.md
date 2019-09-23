## DeepGuider
_DeepGuider Project_ is ...

### DeepGuider Modules
* `core`: Common and basic data structures and interfaces such as pose, path, map, and algorithms
* `map_manager`: A module to interact with cloud maps and paths
* `vps`: Algorithms to find location clues using visual input and street-view images (visual positioning system; VPS)
* `localizer`: Algorithms to estimate accurate robot pose by merging various location clues
* `guidance`: Algorithms to guide the robot to follow the given path and recognize its states
* `exploration`: Algorithms to explore the robot and recover robot pose in degenerate cases such as lost

### Dependency
* [OpenCV](http://opencv.org/) (> 3.0.0, [3-clause BSD License](https://opencv.org/license/))
  * _OpenCV_ is a base for basic computer vision algorithms, linear algebra, image/video manipulation, and GUI.
* [QGC Ground Control Station](https://github.com/mavlink/qgroundcontrol) ([Apache 2.0 License](https://github.com/mavlink/qgroundcontrol/blob/master/COPYING.md))
  * UTM-related routines in _QGC Ground Control Station_ is utilized for conversion between geodesic notation (latitude and longitude) and UTM.
  * It is included in `EXTERNAL` directory.

### How to Run Codes
* **Microsoft Windows with Microsoft Visual Studio**
  * Prerequisite
    * [Microsoft Visual Studio](https://visualstudio.microsoft.com/) (>= 2015; for [binary compatibility](https://docs.microsoft.com/ko-kr/cpp/porting/binary-compat-2015-2017))
    * [OpenCV binaries](https://github.com/sunglok/3dv_tutorial/releases/download/misc/OpenCV_v4.1.1_MSVS2017_x64.zip) (v4.1.1, x64)
  * Clone (or unzip) [DeepGuider codes](https://github.com/deepguider/RoadGPS/archive/master.zip) at `your_folder`
  * Unzip [OpenCV binaries](https://github.com/sunglok/3dv_tutorial/releases/download/misc/OpenCV_v4.1.1_MSVS2017_x64.zip) at `your_folder`
  * Open the MSVS solution file, `your_folder\examples\examples.sln`
  * Run any example and enjoy codes
* **Linux with GCC**
  * Prerequisite
    * [GCC](https://gcc.gnu.org/) and [CMake](https://cmake.org/)
    * [OpenCV](http://opencv.org/): `sudo apt install libopencv-dev python3-opencv`
  * Clone (or unzip) [DeepGuider codes](https://github.com/deepguider/RoadGPS/archive/master.zip) at `your_folder`
  * Build an example at `your_folder/examples/any_example_to_run`: `cmake && make`
  * Run the executable and enjoy codes

### How to Contribute Your Codes
Please refer [CONTRIBUTING.md](CONTRIBUTING.md).

### Authors
Please refer [AUTHORS.md](AUTHORS.md).

### License
Please refer [DeepGuider Project LSA](LICENSE.md).

### Acknowledgement
The authors thank the following contributors and projects.

* This work was supported by the ICT R&D program of [MSIT](https://msit.go.kr/)/[IITP](https://www.iitp.kr/), *Development of AI Technology for Guidance of a Mobile Robot to its Goal with Uncertain Maps in Indoor/Outdoor Environments* (2019-0-01309).
