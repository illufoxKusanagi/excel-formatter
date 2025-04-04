# Excel Formatter
![Excel Formatter is Here!](/icons/excelConvert128.png "Excel Formatter") A lightweight utility for processing Excel (.xlsx) files, optimized for cleaning and reformatting log data.

## Features

- Convert text-formatted numbers to proper number format (recognizes both dot and comma as decimal separators)
- Process multiple sheets in a single file
- Simple drag-and-drop interface
- No installation required - portable executable

## System Requirements

- Windows 7/8/10/11 (64-bit)
- 8 GB RAM or more
- No additional dependencies required

## Download

1. [Click here](https://github.com/illufoxKusanagi/excel-number-formatter/releases) to find the binaries
2. Download the latest `ExcelFormatter.zip` file
3. Extract the contents to any folder on your computer
4. Run `excelFormatter.exe`

## How to Use

### Method 1: Drag and Drop
1. Launch the application
2. Drag an Excel (.xlsx) file from Windows Explorer and drop it onto the application window
3. Select a location to save the processed file
4. Wait for processing to complete

### Method 2: File Menu
1. Launch the application
2. Click "Browse file" button
3. Select an Excel (.xlsx) file to process
4. Choose a save location for the processed file
5. Wait for processing to complete

## What This Tool Does

This application processes Excel files in the following ways:

+ **Number Formatting**: Converts cells containing numeric values stored as text into proper number format
   - Handles both period (.) and comma (,) as decimal separators
   - Example: "123.45" or "123,45" will be converted to 123.45 (numeric value)

## Tips for Best Results

- Make sure Excel files aren't open in another application when processing
- For very large files, processing may take a few minutes and consumes a lot of memory (RAM). Make sure your computer has enough memory to run large files
- The output file will maintain the same structure as the original except for the deleted reformatted numbers
- You should not cancel the process once it started

## Technical Details

- Built with Qt 6 and C++
- Uses QXlsx library for Excel processing. For more details, visit this [QXlsx repository](https://github.com/QtExcel/QXlsx)
- No external dependencies required

## Support

For issues or questions, please [open an issue](https://github.com/illufoxKusanagi/excel-number-formatter/issues) on the GitHub repository.

---

<!-- *This project is maintained by Illufox Kusangi. Released under [License Type].* -->
