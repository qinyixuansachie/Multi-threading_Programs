#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <regex.h>
#include <sys/time.h>
#include <string.h>

enum State { S0, S1, S2, S3};

struct state_count_map{
    enum State state;
    int count;
};

int match = 0;
/*
    checks whether a string segment matches the given rgx
*/
bool is_matching_regex(char* seg, char* rgx_pattern){
    regex_t regex;
    int reti;

    //convert pattern string to regex
    reti = regcomp(&regex, rgx_pattern, 0);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        return false;
    }

    reti = regexec(&regex, seg, 0, NULL, 0);
    if (!reti) {
        regfree(&regex);
        return true;
    } else if (reti == REG_NOMATCH) {
        regfree(&regex);
        return false;
    } else {
        fprintf(stderr, "Regex match failed due to an error\n");
        return false;
    }
}

/*
    manages state transition
*/
void transition(struct state_count_map thread_state, char input) {
    char str[2];
    str[0] = input;
    str[1] = '\0'; // Null terminator
    enum State current_state = thread_state.state;
    switch (current_state) {
        case S0:
            if (is_matching_regex(str, "[1-9]")) thread_state.state = S1;
            else thread_state.state = S0;
        case S1:
            if (is_matching_regex(str, "[1-9a-f]")) thread_state.state = S2;
            else thread_state.state = S0;
        case S2:
            if (is_matching_regex(str, "[0-9a-f]")) thread_state.state = S3;
            else thread_state.state = S0;
        case S3:
            if (is_matching_regex(str, "[0-9a-f]")) thread_state.state = S3;
            else {
                //segment is a match, match ++
                #pragma omp atomic
                thread_state.count++;
                thread_state.state = S0;
            }
        default:
            thread_state.state = S0;
    }
}

/*
    generate random string
*/
char* generate_random_string(int size) {
    const char charset[] = "0011223456789abcdefxxy";
    const int range = strlen(charset);
    char* result_str = malloc((size + 1) * sizeof(char));
    
    srand(time(NULL)); //set seed
    
    for (int i = 0; i < size; i++) {
        result_str[i] = charset[rand() % range]; // Select random character from charset
    }
    result_str[size] = '\0'; // Null-terminate the string
    return result_str;
}

/*
    get segment to be processed by thread
*/
char* get_thread_seg(char* source, int size, int offset){
    char *segment = malloc(size * sizeof(char));
    for (int i = 0; i < size; i++){
        segment[i] = source[offset + i];
    }
    return segment;
}


int get_total_count(struct state_count_map thread_state_0, struct state_count_map** thread_state_map, int t){
    enum State start_state = thread_state_0.state;
    int count = thread_state_0.count;

    for (int i = 0; i < t; i++){
        if (i == 0){
            continue;
        }
        struct state_count_map* thread_states = thread_state_map[i];
        struct state_count_map valid_state;
        switch(start_state){
            case S0: 
                valid_state = thread_states[0];
                break;
            case S1:
                valid_state = thread_states[1];
                break;
            case S2:
                valid_state = thread_states[2];
                break;
            case S3:
                valid_state = thread_states[3];
                break;
            default:
                break;
        }
        start_state = valid_state.state;
        count += valid_state.count;
    }
    return count;
}

int main(int argc,char *argv[]) {
    //handle user input
    int i;
    int t = 8,n = 10;
    if (argc > 1) {
        t = atoi(argv[1]);
        printf("Using %d threads\n",t);
        if (argc>2) {
            n = atoi(argv[2]);
            printf("Testing string of length %d\n",n);
        }
    }

    //generate string
    char* test_str = generate_random_string(n);

    //limit the number of threads to t
    omp_set_dynamic(0);
    omp_set_num_threads(t);

    //some setup
    int segment_size = n / t; //number of chars to be processed per thread
    int last_seg_size = segment_size + n % t; //size of the last segment may be different
    struct state_count_map thread_state_0;
    struct state_count_map** thread_state_map = malloc(t * sizeof(struct state_count_map*));
    thread_state_map[0] = NULL;

    //start timer
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    #pragma omp parallel for
    for (int i = 0; i < t; i++) {
        //thread 0
        if (i == 0){
            thread_state_0.state = S0;    //thread 0 starts from S0
            thread_state_0.count = 0;
            char* seg = get_thread_seg(test_str, segment_size, 0);
            for(int i = 0; i < segment_size; i++){
                transition(thread_state_0, seg[i]);
            }
            free(seg);
        } else{
            int seg_size = (i == t - 1)? last_seg_size: segment_size;
            char* seg = get_thread_seg(test_str, seg_size, i * segment_size);
            struct state_count_map* thread_states = malloc(4 * sizeof(struct state_count_map));
            //generate a map for each initial state
            for(int j = 0; j < 4; j++){
                struct state_count_map thread_state;
                switch (j){
                    case 0:
                        thread_state.state = S0;
                        break;
                    case 1:
                        thread_state.state = S1;
                        break;
                    case 2:
                        thread_state.state = S2;
                        break;
                    case 3:
                        thread_state.state = S3;
                        break;
                    default:
                        break;
                }
                for(int k = 0; k < seg_size; k++){
                    transition(thread_state, seg[k]);
                }
                thread_states[j] = thread_state;
            }
            thread_state_map[i] = thread_states;
            free(seg);
        } 
    }

    int total_matches = get_total_count(thread_state_0, thread_state_map, t);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    uint64_t delta_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
    printf("Input string content: %s\n", test_str);
    printf("Total count of matches: %d\n", total_matches);
    printf("Program takes %lu ms\n", delta_ms);
}