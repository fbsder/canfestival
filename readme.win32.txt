-----------------------------------------------
              CanFestival-3 Win32 port
-----------------------------------------------
14-Feb-2007


1. Installation
-----------------------------------------------
Run configure.win32.bat from CanFestival root directory.


2. Implementation
-----------------------------------------------

CanFestival-3 core code is implemented as standalone dll.
Drivers can be implemented in separated, non-LGPL dll.

Check
drivers\win32\drivers_win32.cpp
and
CanFestival-3\drivers\CAN-uVCCM\CAN-uVCCM.cpp
for implementation details.

One API call was added
BOOL LoadCanDriver(const char* driver_name)
Call you should load your driver dll with this function before making any CanFestival calls.


3. Compatibility
-----------------------------------------------

Code was compiled MS VisualStudio 2003.NET and VisualStudio 2005.NET
for WindowsXP  with ANSI and UNICODE configurations and for WindowsCE 5.0. 
Some preliminary testing was done, but not enough to be used in mission critical projects.


4. Additional Features
-----------------------------------------------
* Non-integral integers support implementation UNS24, UNS40, UNS48 etc.

* When enable debug output with DEBUG_WAR_CONSOLE_ON or DEBUG_ERR_CONSOLE_ON, 
you can navigate in CanFestival source code by double clicking at diagnostic lines in 
VisualStudio.NET 200X Debug Output Window.


5. Object Dictionary Generator Utility Win32 notes
-----------------------------------------------

To run objdictedit.py in Win32, get Python, wxPython and Gnosis Utils 

Python
http://www.activestate.com/products/activepython/
or
http://www.python.org/download/
Get version 2.4. 

wxPython
http://www.wxpython.org/
NOTE: Get wxPython 2.6.3.3. Newer versions will give you an error when you will run objdictgen.py

Gnosis Utils
http://freshmeat.net/projects/gnosisxml/
http://www.gnosis.cx/download/Gnosis_Utils.More/Gnosis_Utils-1.2.1.win32-py24.exe
Get latest version.


Custom size integral types such as INTEGER24, UNS40, INTEGER56 etc.
have been defined as 64 bits integers. You will need to replace sizeof(TYPE)
operators to sizeof_TYPE definitions in generated code, i.e.
replace sizeof(UNS40) with sizeof_UNS40.


Leonid Tochinski
ltochinski@chattenassociates.com
