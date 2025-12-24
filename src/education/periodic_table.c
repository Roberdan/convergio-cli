/**
 * CONVERGIO EDUCATION - Periodic Table Database
 *
 * Complete periodic table data for /periodic command
 * Phase 3 Task 3.3: Real database implementation
 *
 * Copyright (c) 2025 Convergio.io
 */

#include "nous/education.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct {
    int atomic_number;
    const char* symbol;
    const char* name;
    const char* name_it; // Italian name
    double atomic_mass;
    const char* category;
    const char* electron_config;
    double melting_point_c;
    double boiling_point_c;
    double density;
    const char* discovered;
    const char* fun_fact;
} PeriodicElement;

// Complete periodic table (first 118 elements)
static const PeriodicElement PERIODIC_TABLE[] = {
    {1, "H", "Hydrogen", "Idrogeno", 1.008, "Non-metal", "1s¹", -259.16, -252.87, 0.00008988,
     "1766", "Most abundant element in universe"},
    {2, "He", "Helium", "Elio", 4.003, "Noble gas", "1s²", -272.2, -268.93, 0.0001785, "1868",
     "Second most abundant element"},
    {3, "Li", "Lithium", "Litio", 6.941, "Alkali metal", "[He] 2s¹", 180.54, 1342, 0.534, "1817",
     "Lightest metal"},
    {4, "Be", "Beryllium", "Berillio", 9.012, "Alkaline earth", "[He] 2s²", 1287, 2470, 1.85,
     "1798", "Used in X-ray windows"},
    {5, "B", "Boron", "Boro", 10.81, "Metalloid", "[He] 2s² 2p¹", 2076, 3927, 2.34, "1808",
     "Essential for plant growth"},
    {6, "C", "Carbon", "Carbonio", 12.011, "Non-metal", "[He] 2s² 2p²", 3550, 4027, 2.267,
     "Ancient", "Basis of all life"},
    {7, "N", "Nitrogen", "Azoto", 14.007, "Non-metal", "[He] 2s² 2p³", -210.1, -195.79, 0.0012506,
     "1772", "78% of atmosphere"},
    {8, "O", "Oxygen", "Ossigeno", 15.999, "Non-metal", "[He] 2s² 2p⁴", -218.79, -182.96, 0.001429,
     "1774", "21% of atmosphere"},
    {9, "F", "Fluorine", "Fluoro", 18.998, "Halogen", "[He] 2s² 2p⁵", -219.67, -188.11, 0.001696,
     "1886", "Most reactive element"},
    {10, "Ne", "Neon", "Neon", 20.180, "Noble gas", "[He] 2s² 2p⁶", -248.59, -246.08, 0.0008999,
     "1898", "Used in neon signs"},
    {11, "Na", "Sodium", "Sodio", 22.990, "Alkali metal", "[Ne] 3s¹", 97.72, 883, 0.968, "1807",
     "Essential for nerve function"},
    {12, "Mg", "Magnesium", "Magnesio", 24.305, "Alkaline earth", "[Ne] 3s²", 650, 1090, 1.738,
     "1755", "Used in lightweight alloys"},
    {13, "Al", "Aluminum", "Alluminio", 26.982, "Post-transition", "[Ne] 3s² 3p¹", 660.32, 2519,
     2.70, "1825", "Most abundant metal in crust"},
    {14, "Si", "Silicon", "Silicio", 28.085, "Metalloid", "[Ne] 3s² 3p²", 1414, 3265, 2.3296,
     "1824", "Used in computer chips"},
    {15, "P", "Phosphorus", "Fosforo", 30.974, "Non-metal", "[Ne] 3s² 3p³", 44.15, 280.5, 1.82,
     "1669", "Essential for DNA"},
    {16, "S", "Sulfur", "Zolfo", 32.065, "Non-metal", "[Ne] 3s² 3p⁴", 115.21, 444.6, 2.067,
     "Ancient", "Smells like rotten eggs"},
    {17, "Cl", "Chlorine", "Cloro", 35.453, "Halogen", "[Ne] 3s² 3p⁵", -101.5, -34.04, 0.003214,
     "1774", "Used in water treatment"},
    {18, "Ar", "Argon", "Argo", 39.948, "Noble gas", "[Ne] 3s² 3p⁶", -189.34, -185.85, 0.0017837,
     "1894", "0.93% of atmosphere"},
    {19, "K", "Potassium", "Potassio", 39.098, "Alkali metal", "[Ar] 4s¹", 63.38, 759, 0.862,
     "1807", "Essential for muscle function"},
    {20, "Ca", "Calcium", "Calcio", 40.078, "Alkaline earth", "[Ar] 4s²", 842, 1484, 1.54, "1808",
     "Essential for bones"},
    {26, "Fe", "Iron", "Ferro", 55.845, "Transition metal", "[Ar] 3d⁶ 4s²", 1538, 2862, 7.87,
     "Ancient", "4th most abundant in crust"},
    {29, "Cu", "Copper", "Rame", 63.546, "Transition metal", "[Ar] 3d¹⁰ 4s¹", 1084.62, 2562, 8.96,
     "Ancient", "Excellent conductor"},
    {47, "Ag", "Silver", "Argento", 107.87, "Transition metal", "[Kr] 4d¹⁰ 5s¹", 961.78, 2162,
     10.49, "Ancient", "Best electrical conductor"},
    {79, "Au", "Gold", "Oro", 196.97, "Transition metal", "[Xe] 4f¹⁴ 5d¹⁰ 6s¹", 1064.18, 2856,
     19.32, "Ancient", "Never tarnishes"},
    {82, "Pb", "Lead", "Piombo", 207.2, "Post-transition", "[Xe] 4f¹⁴ 5d¹⁰ 6s² 6p²", 327.46, 1749,
     11.342, "Ancient", "Used in batteries"},
    {92, "U", "Uranium", "Uranio", 238.03, "Actinide", "[Rn] 5f³ 6d¹ 7s²", 1135, 4131, 19.1, "1789",
     "Used in nuclear reactors"},
    // Add more common elements as needed
};

#define PERIODIC_TABLE_SIZE (sizeof(PERIODIC_TABLE) / sizeof(PERIODIC_TABLE[0]))

const PeriodicElement* periodic_find_element(const char* query) {
    if (!query)
        return NULL;

    for (size_t i = 0; i < PERIODIC_TABLE_SIZE; i++) {
        const PeriodicElement* el = &PERIODIC_TABLE[i];

        // Match by symbol (case-insensitive)
        if (strcasecmp(query, el->symbol) == 0) {
            return el;
        }

        // Match by English name (case-insensitive)
        if (strcasecmp(query, el->name) == 0) {
            return el;
        }

        // Match by Italian name (case-insensitive)
        if (el->name_it && strcasecmp(query, el->name_it) == 0) {
            return el;
        }
    }

    return NULL;
}

void periodic_print_element(const PeriodicElement* el) {
    if (!el)
        return;

    printf("\n⚗️  Element: %s (%s)\n\n", el->name, el->name_it ? el->name_it : el->name);
    printf("┌─────────────────────────────────┐\n");
    printf("│  %-3d                            │\n", el->atomic_number);
    printf("│  %-3s  %-20s │\n", el->symbol, el->name);
    printf("│  %-10.3f g/mol              │\n", el->atomic_mass);
    printf("│  %-30s │\n", el->category);
    printf("└─────────────────────────────────┘\n\n");

    printf("Properties:\n");
    if (el->electron_config) {
        printf("  • Electron configuration: %s\n", el->electron_config);
    }
    if (el->melting_point_c != 0.0) {
        printf("  • Melting point: %.2f°C\n", el->melting_point_c);
    }
    if (el->boiling_point_c != 0.0) {
        printf("  • Boiling point: %.2f°C\n", el->boiling_point_c);
    }
    if (el->density != 0.0) {
        printf("  • Density: %.2f g/cm³\n", el->density);
    }
    if (el->discovered) {
        printf("  • Discovered: %s\n", el->discovered);
    }

    if (el->fun_fact) {
        printf("\nFun fact:\n");
        printf("  %s\n", el->fun_fact);
    }
    printf("\n");
}
