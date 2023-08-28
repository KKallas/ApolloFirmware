
void readTemp() {
  int temperatureData;

  Wire.requestFrom(78,2);
  while(Wire.available()) {
    temperatureData = (Wire.read() << 8) | Wire.read();  // fill out 2 bytes from I2C comm
    temperatureData = temperatureData >> 5;              // adjust to 11 bit value
    insertIntoTempArray(temperatureData);                // insert into array
  }
}

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