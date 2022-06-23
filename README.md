# kinetic-robo-remix

## description
This project is a new firmware to drive a servo controlled Kinetic Hand with a pressure sensor.

this firmware is a derivative of firmware released by Michele Praga with the "Remix kinetic hand" project.

## Thingiverse projects
Remix kinetic hand by Michele Praga -> https://www.thingiverse.com/thing:4750846

Bionic Hand Muscle Sensor by Michele Praga -> https://www.thingiverse.com/thing:4745915

Kinetic Hand by Matt Bowdell -> https://www.thingiverse.com/thing:4618922

## main features
Added single contraction hand control feature:
1) when the hand is open a single contraction activates hand closing movement, hand closes until contraction is detected
2) when the hand is closed a single contraction activates hand opening movement, hand is completely open
3) contractions below 200ms are ignored for hand state changes

## Supported boards
Arduino nano 33 ble

ESP32 M5StickC

## Authors
Michele Praga - pragamichele@gmail.com

Alberto Navatta - alberto@e-nableitalia.it /  alberto.navatta@gmail.com

## License
This software is released under *Creative Commons - Attribution - Non-Commercial - Share Alike license*

Kinetic Hand by Matt Bowtell and derivative projects are released under *Creative Commons - Attribution - Non-Commercial - Share Alike license*

## LIMITATION OF LIABILITY.
UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL THEORY, WHETHER TORT (INCLUDING NEGLIGENCE), CONTRACT, OR OTHERWISE, SHALL YOU, THE INITIAL DEVELOPER, ANY OTHER CONTRIBUTOR, OR ANY DISTRIBUTOR OF COVERED CODE, OR ANY SUPPLIER OF ANY OF SUCH PARTIES, BE LIABLE TO ANY PERSON FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES OF ANY CHARACTER INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF GOODWILL, WORK STOPPAGE, COMPUTER FAILURE OR MALFUNCTION, OR ANY AND ALL OTHER COMMERCIAL DAMAGES OR LOSSES, EVEN IF SUCH PARTY SHALL HAVE BEEN INFORMED OF THE POSSIBILITY OF SUCH DAMAGES. THIS LIMITATION OF LIABILITY SHALL NOT APPLY TO LIABILITY FOR DEATH OR PERSONAL INJURY RESULTING FROM SUCH PARTY'S NEGLIGENCE TO THE EXTENT APPLICABLE LAW PROHIBITS SUCH LIMITATION. SOME JURISDICTIONS DO NOT ALLOW THE EXCLUSION OR LIMITATION OF INCIDENTAL OR CONSEQUENTIAL DAMAGES, SO THIS EXCLUSION AND LIMITATION MAY NOT APPLY TO YOU.
