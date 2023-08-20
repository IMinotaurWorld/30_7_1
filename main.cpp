#include <chrono>
#include <iostream>
#include <random>
#include "ThreadPool.h"

using namespace std;

mutex coutLocker;
bool thread_mode = true;


void quicksort(vector<int>& array, int left, int right, RequestHandler& tp){
	if (left >= right) return;
	
	int left_bound = left;
    int right_bound = right;
    long middle = array[(left_bound + right_bound) / 2];
    
    do {
        while (array[left_bound] < middle) {
            left_bound++;
        }
        while (array[right_bound] > middle) {
            right_bound--;
        }
        if (left_bound <= right_bound) {
            std::swap(array[left_bound], array[right_bound]);
            left_bound++;
            right_bound--;
        }
    } while (left_bound <= right_bound);
    
    if (thread_mode && (right_bound - left > 100000))
    {
        auto f = tp.push_task(quicksort, std::ref<std::vector<int> >(array), left, right_bound, std::ref<RequestHandler>(tp));
        quicksort(array, left_bound, right, tp);
        while (f.wait_for(std::chrono::seconds(0)) == std::future_status::timeout)
        {
            tp.run_pending_task();		
        }
    }
    else {
        quicksort(array, left, right_bound, tp);
        quicksort(array, left_bound, right, tp);
    }
}

int main(int argc, char** argv) {
	srand(0);
    const int size = 10000000;
    vector<int> arr;
    for (int i = 0; i < size; ++i)
        arr.push_back(rand() % 500);
    time_t start, end;
    // multiple launch
    time(&start);
    RequestHandler rh;
    quicksort(arr, 0, arr.size() - 1, rh);
    time(&end);
    double seconds = difftime(end, start);
    printf("Sort in thread mode: %f seconds\n", seconds);

    for (long i = 0; i < size - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            cout << "Unsorted" << endl;
            break;
        }
    }
    arr.clear();
    for (int i = 0; i < size; ++i)
        arr.push_back(rand() % 500);
    // base launch
    thread_mode = false;
    time(&start);
    quicksort(arr, 0, size - 1, rh);
    time(&end);
    for (long i = 0; i < size - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            cout << "Sort error" << endl;
            break;
        }
    }
    seconds = difftime(end, start);
    printf("Sort in basic mode: %f seconds\n", seconds);
    for (long i = 0; i < size - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            cout << "Sort error" << endl;
            break;
        }
    }
    return 0;
}