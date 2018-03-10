# Med (Memory Editor)

There was a memory editor in Windows, that was Game Master. But it was not a freeware. And there is a freeware memory editor, it is ArtMoney. But it is also for Windows only. In Linux, there is only one memory editor, **scanmem** with GameConqueror as the GUI. However, it does not fulfil my needs. Thus, I decided to create one which can fit my needs.

Med is still in the development, it is not stable. Anyone can help to improve the program.

![Memory editing](http://i.imgur.com/6gSR0WI.png)


## Usage

`med-qt` is a GUI memory editor. In order to use it, please use **sudo**.

Before scanning or opening a JSON file, one must select a target process.


## Interface

The interface can briefly separated into two panes. Left pane is the result from the scan; right pane is the memory addresses that we intended to store and save, or open from the file.


## Scanning & filtering

1. Before scanning, please click "Process" button to choose the process that we want to scan for the memory.
2. After choosing the process, you can type in the value that you want to **scan**. (For the current stage, the only data types allowed are int8, int16, int32, float32, float64, and string.) For example, we can scan for the gold amount.
3. After we make some changes of the gold in the game, you can **filter** it.

## Manage address

The scanned or stored memory addresses, you can

* edit the data type.
* edit the value.

At the right pane, you can

* Use menu to add **new** memory address and edit manually.
* When you select (highlight) a row, you can use `Next` or `Previous` to create next/previous memory address based on the row you selected.
* **delete** the selected memory address with `DEL` key.

## Shifting memory address

Memory are usually dynamically allocated, the memory address will change whenever you start a process. Therefore, we need to shift our saved memory to the new location.

In order to solve this problem, two input fields: `Shift from` and `Shift to / byte` are provided. And three buttons `Shift`, `Unshift`, and `Move` works with the fields.


### Example 1

For example, one of the item, namely Gold, memory address that you stored is 0x20de9b94. After you restart the game, the memory address you scan is changed to 0x20c3cb80.

1. In order to shift the memory, copy-paste 0x20de9b94 to the `Shift from` and 0x20c3cb80 to the `Shift to / byte`.
2. Select the memory address (the Gold) that you want to shift. Multiple selection is allowed.
3. Press `Shift` button.
4. Then all your selected address will be shifted.

`Unshift` is a reverse of `Shift`.

### Example 2

Similar to `Shift` and `Unshift`, let's say you have first character HP memory address located at 0x20de9b90, and the second character HP is located at 0x20de9ba2. Use a calculator that supports hexadecimal, then we can get the difference of 18 bytes.

If you have the memory addresses like HP, MP, strength, wisdom, agility, etc of the first character, then you can move these addresses to the second character location.

1. Fill the 18 at `Shift to / bytes`.
2. Select the rows that we want to move.
3. Press `Move` button.

If we want to move back, fill in with negative value, and press move.


## Scan by array

Let's say we know a hero has the attributes like Max HP, HP, Max MP, and MP, with each 16-bits (2 bytes), then we can scan by array choosing `int16` and enter the values with comma,

`3000, 2580, 1500, 1500`

where the Max HP is 3000, current HP is 2580, Max MP is 1500, and current MP is 1500.


## Save/open file

The JSON file is used. Please save the file in the JSON extension.


## Memory Editor

We can view and edit the memory of a process as hexadecimal values.

1. Go to menu Address > Editor. You should get a popup window.
2. Get the memory address you are interested with, eg: 0x7ffdb8979b90.
3. Paste the address to the **Base** field in the popup window.
4. When your cursor is away from the **Base** field, the hex data should be displayed as in the screenshot.

In the memory editor, **Base** field is the base address of the memory that we are interested.
**Cursor** field is the memory address according to the cursor that is moving.
**Value** is currently read-only value of the cursor.
Left pane is the memory address.
Middle pane is the hex reprensentation of the memory. We can directly make the changes to the memory of the process.
Right pane is the ASCII representation of the memory. It is useful for viewing the string.


## Encoding

Menu View > Encoding allows to change the encoding that we want to read and scan.
It will affect the Memory Editor as well.
Currently only support Big5 where the Default is actually UTF8.

### Usage

For example, if a game uses Big5 encoding, we can change the encoding to Big5 and search the text like "臺灣" (Traditional Chinese).

Note: Qt5 application run as root doesn't support IME like Fcitx. Please use copy-paste instead.


## Search for unknown value (experimental)

If we are interested on a value of the game, but it is not a numerical value, such as a hero of the game is poisoned or normal. We can use unknown search.

0. Suggest to choose the Scan type as "int8".
1. Enter "?", and press "Scan" button. You should get the statusbar showing "Snapshot saved".
2. Now make the changes of the status, like from poisoned to normal or vice versa.
3. Enter "!" which indicates "changed", and press "Filter" button.
4. The scanner will scan for the memory address with the value changed.
5. Continue to filter until you get the potential memory address that handles the status.

Other operators are ">" and "<".
">" with "Filter" will scan for the value that is increased.
"<" with "Filter" will scan for the value that is decreased.


# Build Instruction

This program requires **GCC** (C++ compiler), **Qt5**, and **JSONPP**.

1. In the directory that contains the source code including `CMakeLists.txt`,

```
mkdir build && cd build
cmake ../
make
```

1. To run the GUI, make sure the `*.ui` files are together with the compiled binary files, and enter

`sudo ./med-qt`


# TODO

1. ~~Scan by array.~~
2. ~~Memory editor dialog (view any memory as block).~~
3. Scan within memory block range.
4. Arithmetic operation with prefix notation.
5. Scan by struct, supports data type syntax
6. ~~Scan by string.~~
7. ~~Scan by changes.~~
8. Scan by pointer(?)

# Developer notes

For the process maps, read `man procfs`. To view maps,

```
sudo cat /proc/[pid]/maps
```
