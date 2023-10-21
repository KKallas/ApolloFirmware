int calibration_points[6][9][4] = {
  { // [0]2800K 
    {0, 0, 0, 0}, // 2800 0
    {14, 4, 0, 3}, // 2800 1
    {28, 10, 0, 6}, // 2800 2
    {56, 22, 0, 13}, // 2800 3
    {113, 44, 0, 26}, // 2800 4
    {228, 94, 1, 51}, // 2800 5
    {457, 195, 2, 103}, // 2800 6
    {915, 390, 4, 207}, // 2800 7
    {1901, 731, 9, 409} // 2800 8
  },
  { // [1]3200K
    {0, 0, 0, 0}, // 3200 0
    {11, 3, 0, 4}, // 3200 1
    {22, 8, 0, 8}, // 3200 2
    {45, 19, 0, 17}, // 3200 3
    {90, 39, 0, 35}, // 3200 4
    {182, 83, 1, 69}, // 3200 5
    {364, 168, 2, 139}, // 3200 6
    {729, 336, 4, 278}, // 3200 7
    {1458, 668, 9, 556} // 3200 8
  },
  { //[2]4800K
    {0, 0, 0, 0}, // 4800 0
    {9, 2, 0, 17}, // 4800 1
    {18, 8, 0, 34}, // 4800 2
    {37, 19, 1, 68}, // 4800 3
    {75, 41, 3, 136}, // 4800 4
    {151, 89, 6, 273}, // 4800 5
    {330, 201, 13, 519}, // 4800 6
    {665, 409, 27, 1035}, // 4800 7
    {1720, 894, 78, 1801} // 4800 8
  },
  { //[3]5600K
    {0, 0, 0, 0}, // 5600 0
    {5, 1, 0, 19}, // 5600 1
    {12, 6, 1, 36}, // 5600 2
    {25, 14, 3, 72}, // 5600 3
    {57, 37, 7, 139}, // 5600 4
    {119, 82, 14, 274}, // 5600 5
    {260, 175, 29, 527}, // 5600 6
    {524, 348, 59, 1051}, // 5600 7
    {992, 650, 105, 2047} // 5600 8
  },
  { //[4]7800K
    {0, 0, 0, 0}, // 7800 0
    {5, 3, 2, 16}, // 7800 1
    {10, 9, 5, 33}, // 7800 2
    {21, 20, 10, 66}, // 7800 3
    {45, 47, 21, 131}, // 7800 4
    {90, 103, 42, 263}, // 7800 5
    {181, 209, 85, 526}, // 7800 6
    {363, 418, 170, 1052}, // 7800 7
    {767, 800, 329, 1968} // 7800 8
  },
  { //[5]10000K
    {0, 0, 0, 0}, // 10000 0
    {4, 4, 4, 16}, // 10000 1
    {9, 9, 7, 32}, // 10000 2
    {18, 23, 14, 64}, // 10000 3
    {42, 53, 29, 122}, // 10000 4
    {86, 114, 59, 243}, // 10000 5
    {173, 235, 119, 487}, // 10000 6
    {344, 435, 239, 977}, // 10000 7
    {673, 847, 472, 1922} // 10000 8
  }
};

// 1 36 73 109 145 181 218 255

void update_calib(int intensity, int wb) {
  int int_index = findLastSmallerInt(intensity);
  int wb_index = findLastSmallerWb(wb);
  Serial.printf("wb_index: %i\n",wb_index);

  // Calibration A
  if (intensity == 0) {
    current_calibration_A[0] = calibration_points[wb_index][0][0];
    current_calibration_A[1] = calibration_points[wb_index][0][1];
    current_calibration_A[2] = calibration_points[wb_index][0][2];
    current_calibration_A[3] = calibration_points[wb_index][0][3];
    return;
  }
  if (intensity == 255) {
    current_calibration_A[0] = calibration_points[wb_index][8][0];
    current_calibration_A[1] = calibration_points[wb_index][8][1];
    current_calibration_A[2] = calibration_points[wb_index][8][2];
    current_calibration_A[3] = calibration_points[wb_index][8][3];
    return;
  }

  for(int i = 0; i < 4; i++) {
    current_calibration_A[i] = map(intensity, intesity_list[int_index], intesity_list[int_index + 1], calibration_points[wb_index][int_index][i], calibration_points[wb_index][int_index + 1][i]);
  }

  // Calibration B
  wb_index = wb_index + 1; 
  if (wb_index > 5) {
    wb_index = 5;
  }

  if (intensity == 0) {
    current_calibration_B[0] = calibration_points[wb_index][0][0];
    current_calibration_B[1] = calibration_points[wb_index][0][1];
    current_calibration_B[2] = calibration_points[wb_index][0][2];
    current_calibration_B[3] = calibration_points[wb_index][0][3];
    return;
  }
  if (intensity == 255) {
    current_calibration_B[0] = calibration_points[wb_index][8][0];
    current_calibration_B[1] = calibration_points[wb_index][8][1];
    current_calibration_B[2] = calibration_points[wb_index][8][2];
    current_calibration_B[3] = calibration_points[wb_index][8][3];
    return;
  }

  for(int i = 0; i < 4; i++) {
    current_calibration_B[i] = map(intensity, intesity_list[int_index], intesity_list[int_index + 1], calibration_points[wb_index][int_index][i], calibration_points[wb_index][int_index + 1][i]);
  }

  // 2800-3200-4800-5600-7800-10000 = 7200K
  // 0    400  2000 2800 5000 7200 
  // 0    14   71   100  178  255
  current_calibration_mixed[0] = map(wb,wb_index_list[wb_index-1],wb_index_list[wb_index],current_calibration_A[0],current_calibration_B[0]);
  current_calibration_mixed[1] = map(wb,wb_index_list[wb_index-1],wb_index_list[wb_index],current_calibration_A[1],current_calibration_B[1]);
  current_calibration_mixed[2] = map(wb,wb_index_list[wb_index-1],wb_index_list[wb_index],current_calibration_A[2],current_calibration_B[2]);
  current_calibration_mixed[3] = map(wb,wb_index_list[wb_index-1],wb_index_list[wb_index],current_calibration_A[3],current_calibration_B[3]);
}

int findLastSmallerInt(int inputIntensity) {
  int lastIndex = -1;  // Initialize with an invalid index

  for (int i = 0; i < 9; i++) {
    if (inputIntensity > intesity_list[i]) {
      lastIndex = i;  // Update the last index if the condition is met
    }
  }

  return lastIndex;
}

int findLastSmallerWb(int inputWb) {
  int lastIndex = -1;  // Initialize with an invalid index

  for (int i = 0; i < 6; i++) {
    if (inputWb > wb_index_list[i]) {
      lastIndex = i;  // Update the last index if the condition is met
    }
  }

  return lastIndex;
}

void set_dmx(int r_in, int g_in, int b_in, int wb_in, int temp_in) {
  // TODO temp
  // Initialize lowest_value with r_in initially
  int lowest_value = r_in;

  // Compare with g_in
  if (g_in < lowest_value) {
    lowest_value = g_in;
  }

  // Compare with b_in
  if (b_in < lowest_value) {
    lowest_value = b_in;
  }

  update_calib(lowest_value, wb_in);

  // Mix in the color
  // TODO convert the RGB gamma, use LUT instead of multiply
  current_calibration_mixed[0] = (r_in - lowest_value) * 8 + current_calibration_mixed[0];
  current_calibration_mixed[1] = (g_in - lowest_value) * 8 + current_calibration_mixed[1];
  current_calibration_mixed[2] = (b_in - lowest_value) * 8 + current_calibration_mixed[2];
}