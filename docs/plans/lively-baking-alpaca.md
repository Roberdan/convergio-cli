# Plan: ESC Key Interruption During API Calls

## Problem
When the user sends a request and the spinner shows "pensando...", there's no way to interrupt the operation. The curl API call blocks for up to 120 seconds.

## Solution Overview
Add ESC key detection during API calls using curl's progress callback mechanism combined with non-blocking terminal input checking.

## Implementation Steps

### 1. Add global cancel flag in `claude.c`
```c
static volatile sig_atomic_t g_request_cancelled = 0;

void claude_cancel_request(void) {
    g_request_cancelled = 1;
}

void claude_reset_cancel(void) {
    g_request_cancelled = 0;
}
```

### 2. Add progress callback to check for cancellation
```c
static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow) {
    (void)clientp; (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
    if (g_request_cancelled) {
        return 1;  // Non-zero aborts the transfer
    }
    return 0;
}
```

### 3. Enable progress callback in all curl calls
Add to each curl setup before `curl_easy_perform()`:
```c
curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
```

### 4. Modify spinner thread to check for ESC key
In `main.c`, modify `spinner_func()` to poll for ESC:
```c
static void* spinner_func(void* arg) {
    // Set terminal to non-blocking
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    new_term.c_cc[VMIN] = 0;
    new_term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    while (g_spinner_active) {
        // Check for ESC key (27)
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1 && c == 27) {
            claude_cancel_request();
            break;
        }
        // Show spinner frame...
        usleep(80000);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    return NULL;
}
```

### 5. Handle cancellation result
After `curl_easy_perform()` returns, check if it was cancelled:
```c
if (res == CURLE_ABORTED_BY_CALLBACK) {
    printf("\n⚠️  Request cancelled\n");
    // cleanup...
    return NULL;
}
```

### 6. Reset cancel flag before each request
In `process_natural_input()`:
```c
claude_reset_cancel();
spinner_start();
response = orchestrator_process(input);
spinner_stop();
```

## Files to Modify

1. **src/neural/claude.c**
   - Add `g_request_cancelled` flag
   - Add `claude_cancel_request()` and `claude_reset_cancel()` functions
   - Add `progress_callback()` function
   - Add progress callback to all 4 `curl_easy_perform()` locations
   - Handle `CURLE_ABORTED_BY_CALLBACK` result

2. **include/nous/nous.h**
   - Declare `claude_cancel_request()` and `claude_reset_cancel()`

3. **src/core/main.c**
   - Add `#include <termios.h>`
   - Modify `spinner_func()` to detect ESC key
   - Call `claude_reset_cancel()` before API calls

## User Experience
- While "pensando..." spinner is active, user presses ESC
- Spinner stops, shows "⚠️ Request cancelled"
- User returns to prompt immediately
- No API charges for cancelled requests (if cancelled before response)
