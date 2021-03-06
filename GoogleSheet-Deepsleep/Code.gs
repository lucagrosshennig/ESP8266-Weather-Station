//-----------------------------------------------
// Author: Luca Großhennig
// Email: luca.grosshennig@gmx.de
// Description: This code for demonstration send data from ESP8266 into Google Spreadsheet
// GET request syntax:
// https://script.google.com/macros/s/<gscript id>/exec?header_here=data_here
//-----------------------------------------------
/**
* Function doGet: Parse received data from GET request, 
  get and store data which is corresponding with header row in Google Spreadsheet
*/

function doGet(e) { 
  Logger.log( JSON.stringify(e) );  // view parameters
  var result = 'Ok'; // assume success
  if (e.parameter == 'undefined') {
    result = 'No Parameters';
  }
  else {
    var sheet_id = '***'; 		// Spreadsheet ID
    var sheet = SpreadsheetApp.openById(sheet_id).getActiveSheet();		// get Active sheet
    if (e.parameter != 'undefined'){
      var newRow = sheet.getLastRow() + 1;						
      var rowData = [];
      var currentdate = new Date(); 
      rowData[0] = currentdate.getDate() + "/"
      + (currentdate.getMonth()+1)  + "/" 
      + currentdate.getFullYear() + " "  
      + currentdate.getHours() + ":"  
      + currentdate.getMinutes();										// Timestamp in column A
      
      for (var param in e.parameter) {
        Logger.log('In for loop, param=' + param);
        var value = stripQuotes(e.parameter[param]);
        Logger.log(param + ':' + e.parameter[param]);
        switch (param) {
          case 'time': //Parameter
            rowData[0] = value; //Value in column A
            result += ' ,Written on column A';
            break;
          case 'temp': //Parameter
            rowData[1] = value; //Value in column B
            result += 'Written on column B';
            break;
          case 'hum': //Parameter
            rowData[2] = value; //Value in column C
            result += ' ,Written on column C';
            break;
          case 'press': //Parameter
            rowData[3] = value; //Value in column D
            result += ' ,Written on column D';
            break;
          case 'light': //Parameter
            rowData[4] = value; //Value in column D
            result += ' ,Written on column E';
            break;
          case 'rain': //Parameter
            rowData[5] = value; //Value in column D
            result += ' ,Written on column F';
            break;
          default:
            result = "unsupported parameter";
        }
      }
      Logger.log(JSON.stringify(rowData));
    // Write new row below
    var newRange = sheet.getRange(newRow, 1, 1, rowData.length);
    newRange.setValues([rowData]);
    }
    else{
      result = 'No parameters defined';
    }
    
  }
  // Return result of operation
  return ContentService.createTextOutput(result);
}
/**
* Remove leading and trailing single or double quotes
*/
function stripQuotes( value ) {
  return value.replace(/^["']|['"]$/g, "");
}
