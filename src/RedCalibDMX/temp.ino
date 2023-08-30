/**
 * Reads temperature data from an I2C device and inserts it into the TempArray.
 *
 * This function communicates with an I2C device to retrieve temperature data, typically from an LM75 sensor.
 * It requests 2 bytes of data from the device and then combines them to form a 16-bit temperature value.
 * The value is then shifted to adjust it to an 11-bit format, suitable for further processing.
 * The adjusted temperature value is then inserted into the TempArray using the insertIntoTempArray function.
 * This function is typically called periodically to update the temperature data and maintain a record of it.
 */
void readTemp() {
  int temperatureData;

  Wire.requestFrom(78,2);
  while(Wire.available()) {
    temperatureData = (Wire.read() << 8) | Wire.read();  // fill out 2 bytes from I2C comm
    temperatureData = temperatureData >> 5;              // adjust to 11 bit value
    insertIntoTempArray(temperatureData);                // insert into array
  }
}

/**
 * Inserts a new temperature value into the circular TempArray and find currentTempData.
 *
 * This function updates the TempArray with a new temperature value using a round-robin buffer mechanism.
 * Additionally, it maintains a sorted copy of the TempArray to efficiently find the middle value. The
 * TempArray is managed in a circular manner. When a new value is added, the array is copied and sorted.
 * The middle value from the sorted array is then assigned to the global variable currentTempData.
 * This filtering approach helps mitigate the impact of occasional erratic readings from the LM75 sensor.
 *
 * @param newValue The new temperature value to be inserted into the TempArray.
 */
void insertIntoTempArray(int newValue) {
  TempArray[nextTempArrayPos] = newValue;                // Round robin buffer to record temp values
  nextTempArrayPos++;
  if (nextTempArrayPos > 4) {
    nextTempArrayPos = 0;                                // Wrap around when reaching end
  }
                                    
  memcpy(sortedArray, TempArray, sizeof(TempArray));     // Create a copy of the array and sort it
  sortArray(sortedArray);
  
  currentTempData = sortedArray[2];                      // Pick the second smallest value as currentTempData
}

/**
 * Sorts an array of unsigned integers in ascending order using the selection sort algorithm.
 *
 * This function iterates through the given array and arranges the elements in ascending order.
 * It uses the selection sort algorithm, which repeatedly selects the smallest element and swaps it
 * with the element at the current position.
 *
 * @param arr The array of unsigned integers to be sorted.
 */
void sortArray(uint arr[]) {
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (arr[j] < arr[i]) {
        uint temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }
  }
}