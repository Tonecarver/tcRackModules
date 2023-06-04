#ifndef _tc_sorting_h_
#define _tc_sorting_h_ 1

// Bubble sort 
void sortFloatAscending(int numMembers, float * values) {
    int j, k;
    for (j = 0; j < (numMembers - 1); j++) {    
        // Last i elements are already in place
        for (k = 0; k < (numMembers - j - 1); k++) {
            if (values[k] > values[k + 1]) {
                // swap these elements
                float tmp = values[k];
                values[k] = values[k + 1];
                values[k + 1] = tmp;
            }
        }
    }
}


#endif // _tc_sorting_h_