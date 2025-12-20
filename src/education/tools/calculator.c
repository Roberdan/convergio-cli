/**
 * CONVERGIO EDUCATION - VISUAL CALCULATOR
 *
 * Step-by-step calculator with color coding for dyscalculia support,
 * visual blocks for place values, and fraction visualization.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// ============================================================================
// CONSTANTS AND COLORS
// ============================================================================

// ANSI color codes for terminal output
#define COLOR_RESET     "\033[0m"
#define COLOR_UNITS     "\033[34m"   // Blue
#define COLOR_TENS      "\033[32m"   // Green
#define COLOR_HUNDREDS  "\033[31m"   // Red
#define COLOR_THOUSANDS "\033[35m"   // Magenta
#define COLOR_DECIMAL   "\033[33m"   // Yellow
#define COLOR_NEGATIVE  "\033[36m"   // Cyan
#define COLOR_OPERATOR  "\033[1m"    // Bold

// ============================================================================
// TYPES
// ============================================================================

typedef struct {
    bool use_colors;
    bool show_blocks;
    bool show_every_step;
    bool use_visual_fractions;
    bool speak_steps;
    bool no_timer;
} CalculatorAccessibility;

typedef enum {
    CALC_ADD,
    CALC_SUBTRACT,
    CALC_MULTIPLY,
    CALC_DIVIDE,
    CALC_POWER,
    CALC_SQRT,
    CALC_PERCENT
} CalcOperation;

typedef struct {
    char* step_description;
    char* visual;
    double intermediate_result;
} CalcStep;

typedef struct {
    double operand1;
    double operand2;
    CalcOperation operation;
    double result;
    CalcStep* steps;
    int step_count;
} Calculation;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static void calc_solve_linear(double a, double b, double c,
                              const CalculatorAccessibility* access);

// ============================================================================
// ACCESSIBILITY SETTINGS
// ============================================================================

static CalculatorAccessibility get_calc_accessibility(const EducationAccessibility* a) {
    CalculatorAccessibility ca = {
        .use_colors = true,
        .show_blocks = false,
        .show_every_step = false,
        .use_visual_fractions = false,
        .speak_steps = false,
        .no_timer = false
    };

    if (!a) return ca;

    // Dyscalculia - full visual support
    if (a->dyscalculia) {
        ca.show_blocks = true;
        ca.show_every_step = true;
        ca.use_visual_fractions = true;
        ca.no_timer = true;
        if (a->dyscalculia_severity >= SEVERITY_MODERATE) {
            ca.speak_steps = a->tts_enabled;
        }
    }

    // Dyslexia - use TTS for steps
    if (a->dyslexia && a->tts_enabled) {
        ca.speak_steps = true;
    }

    // High contrast mode
    if (a->high_contrast) {
        ca.use_colors = true; // Colors help with high contrast
    }

    return ca;
}

// ============================================================================
// COLOR-CODED NUMBER DISPLAY
// ============================================================================

/**
 * Display a number with place-value color coding
 * Units=Blue, Tens=Green, Hundreds=Red, Thousands=Magenta
 */
void calc_print_colored_number(double num, bool use_colors) {
    if (!use_colors) {
        printf("%.2f", num);
        return;
    }

    // Handle negative
    if (num < 0) {
        printf("%s-%s", COLOR_NEGATIVE, COLOR_RESET);
        num = -num;
    }

    // Split into integer and decimal parts
    long long integer_part = (long long)num;
    double decimal_part = num - integer_part;

    // Print integer part with colors
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", integer_part);
    int len = strlen(buf);

    for (int i = 0; i < len; i++) {
        int place = len - i - 1;
        const char* color;

        switch (place % 3) {
            case 0: color = COLOR_UNITS; break;
            case 1: color = COLOR_TENS; break;
            case 2: color = COLOR_HUNDREDS; break;
            default: color = COLOR_THOUSANDS; break;
        }

        // Thousands use magenta
        if (place >= 3) color = COLOR_THOUSANDS;

        printf("%s%c%s", color, buf[i], COLOR_RESET);
    }

    // Print decimal part
    if (decimal_part > 0.001) {
        printf("%s.%s", COLOR_DECIMAL, COLOR_RESET);
        snprintf(buf, sizeof(buf), "%.2f", decimal_part);
        // Skip "0."
        printf("%s%s%s", COLOR_DECIMAL, buf + 2, COLOR_RESET);
    }
}

/**
 * Display number as visual blocks (for dyscalculia)
 */
void calc_print_blocks(int num) {
    if (num < 0 || num > 999) {
        printf("[%d]", num);
        return;
    }

    int hundreds = num / 100;
    int tens = (num % 100) / 10;
    int units = num % 10;

    printf("\n  ");

    // Hundreds as large squares
    if (hundreds > 0) {
        printf("%s", COLOR_HUNDREDS);
        for (int i = 0; i < hundreds; i++) printf("[100]");
        printf("%s ", COLOR_RESET);
    }

    // Tens as lines
    if (tens > 0) {
        printf("%s", COLOR_TENS);
        for (int i = 0; i < tens; i++) printf("[10]");
        printf("%s ", COLOR_RESET);
    }

    // Units as dots
    if (units > 0) {
        printf("%s", COLOR_UNITS);
        for (int i = 0; i < units; i++) printf("[1]");
        printf("%s", COLOR_RESET);
    }

    printf("\n");
}

// ============================================================================
// STEP-BY-STEP CALCULATIONS
// ============================================================================

/**
 * Addition with step-by-step explanation
 */
Calculation* calc_add_steps(double a, double b, const CalculatorAccessibility* access) {
    Calculation* calc = calloc(1, sizeof(Calculation));
    if (!calc) return NULL;

    calc->operand1 = a;
    calc->operand2 = b;
    calc->operation = CALC_ADD;
    calc->result = a + b;

    // Generate steps
    if (access && access->show_every_step) {
        // For integers, show place-value addition
        if (a == (int)a && b == (int)b && a >= 0 && b >= 0 && a < 1000 && b < 1000) {
            int ia = (int)a;
            int ib = (int)b;

            int units_a = ia % 10;
            int tens_a = (ia / 10) % 10;
            int hundreds_a = ia / 100;

            int units_b = ib % 10;
            int tens_b = (ib / 10) % 10;
            int hundreds_b = ib / 100;

            // Maximum 4 steps for simple addition
            calc->steps = calloc(4, sizeof(CalcStep));
            calc->step_count = 0;

            // Step 1: Show the numbers
            calc->steps[calc->step_count].step_description =
                strdup("Primo passo: separiamo i numeri per posizione");
            char visual[256];
            snprintf(visual, sizeof(visual),
                     "  %s%d%s%s%d%s%s%d%s\n"
                     "+ %s%d%s%s%d%s%s%d%s\n"
                     "--------",
                     COLOR_HUNDREDS, hundreds_a, COLOR_RESET,
                     COLOR_TENS, tens_a, COLOR_RESET,
                     COLOR_UNITS, units_a, COLOR_RESET,
                     COLOR_HUNDREDS, hundreds_b, COLOR_RESET,
                     COLOR_TENS, tens_b, COLOR_RESET,
                     COLOR_UNITS, units_b, COLOR_RESET);
            calc->steps[calc->step_count].visual = strdup(visual);
            calc->step_count++;

            // Step 2: Add units
            int units_sum = units_a + units_b;
            int carry_to_tens = units_sum / 10;
            units_sum = units_sum % 10;

            snprintf(visual, sizeof(visual),
                     "Unita: %s%d%s + %s%d%s = %s%d%s%s",
                     COLOR_UNITS, units_a, COLOR_RESET,
                     COLOR_UNITS, units_b, COLOR_RESET,
                     COLOR_UNITS, units_a + units_b, COLOR_RESET,
                     carry_to_tens ? " (riporto 1 alle decine)" : "");
            calc->steps[calc->step_count].step_description = strdup(visual);
            calc->step_count++;

            // Step 3: Add tens
            int tens_sum = tens_a + tens_b + carry_to_tens;
            int carry_to_hundreds = tens_sum / 10;
            tens_sum = tens_sum % 10;

            snprintf(visual, sizeof(visual),
                     "Decine: %s%d%s + %s%d%s + %d = %s%d%s%s",
                     COLOR_TENS, tens_a, COLOR_RESET,
                     COLOR_TENS, tens_b, COLOR_RESET,
                     carry_to_tens,
                     COLOR_TENS, tens_a + tens_b + carry_to_tens, COLOR_RESET,
                     carry_to_hundreds ? " (riporto 1 alle centinaia)" : "");
            calc->steps[calc->step_count].step_description = strdup(visual);
            calc->step_count++;

            // Step 4: Result
            snprintf(visual, sizeof(visual),
                     "\nRisultato: ");
            calc->steps[calc->step_count].step_description = strdup(visual);
            calc->steps[calc->step_count].intermediate_result = calc->result;
            calc->step_count++;
        }
    }

    return calc;
}

/**
 * Subtraction with step-by-step explanation
 */
Calculation* calc_subtract_steps(double a, double b, const CalculatorAccessibility* access) {
    Calculation* calc = calloc(1, sizeof(Calculation));
    if (!calc) return NULL;

    calc->operand1 = a;
    calc->operand2 = b;
    calc->operation = CALC_SUBTRACT;
    calc->result = a - b;

    // Similar step generation as addition
    // TODO: Implement detailed steps

    return calc;
}

/**
 * Multiplication with step-by-step explanation
 */
Calculation* calc_multiply_steps(double a, double b, const CalculatorAccessibility* access) {
    Calculation* calc = calloc(1, sizeof(Calculation));
    if (!calc) return NULL;

    calc->operand1 = a;
    calc->operand2 = b;
    calc->operation = CALC_MULTIPLY;
    calc->result = a * b;

    if (access && access->show_every_step && a == (int)a && b == (int)b) {
        // Show multiplication as repeated addition or grid
        // TODO: Implement detailed steps
    }

    return calc;
}

/**
 * Division with step-by-step explanation
 */
Calculation* calc_divide_steps(double a, double b, const CalculatorAccessibility* access) {
    Calculation* calc = calloc(1, sizeof(Calculation));
    if (!calc) return NULL;

    calc->operand1 = a;
    calc->operand2 = b;
    calc->operation = CALC_DIVIDE;

    if (b == 0) {
        // Division by zero - use a sentinel value
        calc->result = 0.0 / 0.0;  // Produces NaN without macro
    } else {
        calc->result = a / b;
    }

    return calc;
}

// ============================================================================
// FRACTION VISUALIZATION
// ============================================================================

/**
 * Display fraction as pizza slices (for dyscalculia)
 */
void calc_print_fraction_visual(int numerator, int denominator) {
    if (denominator <= 0 || denominator > 12) {
        printf("%d/%d", numerator, denominator);
        return;
    }

    printf("\n  ");

    // Draw a simple ASCII "pizza"
    for (int i = 0; i < denominator; i++) {
        if (i < numerator) {
            printf("[#]"); // Filled slice
        } else {
            printf("[ ]"); // Empty slice
        }
    }

    printf(" = %d/%d", numerator, denominator);

    // Show decimal equivalent
    printf(" = %.2f", (double)numerator / (double)denominator);
    printf("\n");
}

/**
 * Compare fractions visually
 */
void calc_compare_fractions(int num1, int den1, int num2, int den2) {
    printf("\nConfronto:\n");

    // First fraction
    printf("  ");
    for (int i = 0; i < den1; i++) {
        printf(i < num1 ? "[#]" : "[ ]");
    }
    printf(" = %d/%d\n", num1, den1);

    // Second fraction
    printf("  ");
    for (int i = 0; i < den2; i++) {
        printf(i < num2 ? "[#]" : "[ ]");
    }
    printf(" = %d/%d\n", num2, den2);

    // Comparison
    double v1 = (double)num1 / den1;
    double v2 = (double)num2 / den2;

    if (fabs(v1 - v2) < 0.0001) {
        printf("  Sono uguali!\n");
    } else if (v1 > v2) {
        printf("  %d/%d e maggiore\n", num1, den1);
    } else {
        printf("  %d/%d e maggiore\n", num2, den2);
    }
}

// ============================================================================
// EQUATION SOLVER
// ============================================================================

/**
 * Parse and solve equation string
 * Supports: ax + b = c, ax = c, x + b = c
 */
int calc_solve_equation(const char* equation,
                        const CalculatorAccessibility* access) {
    if (!equation) return -1;

    // Simple parser for linear equations
    // Format: "2x + 3 = 7" or "x - 5 = 10"
    double a = 0, b = 0, c = 0;
    char eq_copy[256];
    strncpy(eq_copy, equation, sizeof(eq_copy) - 1);

    // Find the equals sign
    char* equals = strchr(eq_copy, '=');
    if (!equals) {
        printf("Error: No equals sign found in equation\n");
        return -1;
    }

    *equals = '\0';
    char* left_side = eq_copy;
    char* right_side = equals + 1;

    // Parse right side (should be a number)
    c = atof(right_side);

    // Parse left side (ax + b format)
    char* x_pos = strchr(left_side, 'x');
    if (!x_pos) {
        printf("Error: No variable 'x' found\n");
        return -1;
    }

    // Get coefficient of x
    char* ptr = left_side;
    while (*ptr == ' ') ptr++;  // Skip whitespace

    if (ptr == x_pos) {
        a = 1.0;  // Just 'x'
    } else if (ptr + 1 == x_pos && *ptr == '-') {
        a = -1.0;  // '-x'
    } else {
        a = atof(ptr);
    }

    // Get constant term (after x)
    char* after_x = x_pos + 1;
    while (*after_x == ' ') after_x++;

    if (*after_x == '+' || *after_x == '-') {
        b = atof(after_x);
    }

    // Solve using calc_solve_linear
    calc_solve_linear(a, b, c, access);

    return 0;
}

/**
 * Solve linear equation ax + b = c with steps
 */
static void calc_solve_linear(double a, double b, double c,
                              const CalculatorAccessibility* access) {
    printf("\nRisolviamo: %.2fx + %.2f = %.2f\n\n", a, b, c);

    if (a == 0) {
        if (b == c) {
            printf("L'equazione e sempre vera (infiniti soluzioni)\n");
        } else {
            printf("L'equazione e impossibile (nessuna soluzione)\n");
        }
        return;
    }

    // Step 1: Isolate the x term
    printf("Passo 1: Spostiamo %.2f dall'altro lato\n", b);
    printf("  %.2fx = %.2f - %.2f\n", a, c, b);
    double right = c - b;
    printf("  %.2fx = %.2f\n\n", a, right);

    // Step 2: Divide by coefficient
    printf("Passo 2: Dividiamo entrambi i lati per %.2f\n", a);
    printf("  x = %.2f / %.2f\n", right, a);
    double x = right / a;
    printf("  x = %.2f\n\n", x);

    // Step 3: Verify
    printf("Verifica: %.2f * %.2f + %.2f = %.2f ✓\n",
           a, x, b, a * x + b);
}

// ============================================================================
// CLEANUP
// ============================================================================

void calc_free(Calculation* calc) {
    if (!calc) return;
    if (calc->steps) {
        for (int i = 0; i < calc->step_count; i++) {
            free(calc->steps[i].step_description);
            free(calc->steps[i].visual);
        }
        free(calc->steps);
    }
    free(calc);
}

// ============================================================================
// CLI COMMAND HANDLER
// ============================================================================

int calculator_command_handler(int argc, char** argv,
                                const EducationStudentProfile* profile) {
    if (argc < 2) {
        printf("Usage: /calc <expression>\n");
        printf("       /calc fraction <num> <den>\n");
        printf("       /calc solve <a>x + <b> = <c>\n");
        printf("       /calc blocks <number>\n");
        return 1;
    }

    const EducationAccessibility* a = profile ? profile->accessibility : NULL;
    CalculatorAccessibility access = get_calc_accessibility(a);

    const char* cmd = argv[1];

    if (strcmp(cmd, "blocks") == 0 && argc >= 3) {
        int num = atoi(argv[2]);
        printf("\nNumero: ");
        calc_print_colored_number(num, access.use_colors);
        calc_print_blocks(num);
        return 0;
    }

    if (strcmp(cmd, "fraction") == 0 && argc >= 4) {
        int num = atoi(argv[2]);
        int den = atoi(argv[3]);
        calc_print_fraction_visual(num, den);
        return 0;
    }

    if (strcmp(cmd, "solve") == 0 && argc >= 5) {
        // Parse ax + b = c
        double a_coef = atof(argv[2]);
        double b_coef = atof(argv[3]);
        double c_val = atof(argv[4]);
        calc_solve_linear(a_coef, b_coef, c_val, &access);
        return 0;
    }

    // Simple expression evaluation
    // For now, just parse "a + b" format
    if (argc >= 4) {
        double a_val = atof(argv[1]);
        const char* op = argv[2];
        double b_val = atof(argv[3]);

        Calculation* calc = NULL;

        if (strcmp(op, "+") == 0) {
            calc = calc_add_steps(a_val, b_val, &access);
        } else if (strcmp(op, "-") == 0) {
            calc = calc_subtract_steps(a_val, b_val, &access);
        } else if (strcmp(op, "*") == 0 || strcmp(op, "x") == 0) {
            calc = calc_multiply_steps(a_val, b_val, &access);
        } else if (strcmp(op, "/") == 0) {
            calc = calc_divide_steps(a_val, b_val, &access);
        }

        if (calc) {
            // Print steps
            for (int i = 0; i < calc->step_count; i++) {
                if (calc->steps[i].step_description) {
                    printf("%s\n", calc->steps[i].step_description);
                }
                if (calc->steps[i].visual) {
                    printf("%s\n", calc->steps[i].visual);
                }
            }

            printf("\n= ");
            calc_print_colored_number(calc->result, access.use_colors);
            printf("\n");

            if (access.show_blocks && calc->result > 0 && calc->result < 1000) {
                calc_print_blocks((int)calc->result);
            }

            calc_free(calc);
        }
    }

    return 0;
}
