#!/bin/bash
# ============================================================================
# FULL LESSON SIMULATION TEST
#
# Simulates a complete lesson between Euclide and Mario, a student with
# dyslexia and dyscalculia, learning to solve equations with powers.
#
# This test validates:
# - Accessibility adaptations (dyslexia, dyscalculia)
# - Maieutic method (guiding questions, not direct answers)
# - Step-by-step progression
# - Positive reinforcement
# - Age-appropriate content
#
# Copyright (c) 2025 Convergio.io
# ============================================================================

set -e

CONVERGIO="./build/bin/convergio-edu"
LOG_FILE="logs/full_lesson_mario_$(date +%Y%m%d_%H%M%S).log"
mkdir -p logs

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  FULL LESSON SIMULATION: Mario learns equations with powers${NC}"
echo -e "${CYAN}  Student: Mario, 13 years old, with dyslexia and dyscalculia${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
echo ""

# Function to send message and capture full response
send_message() {
    local agent="$1"
    local message="$2"
    local description="$3"

    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}[STUDENT MARIO]:${NC} $message"
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    # Send to convergio and capture full response
    echo "@$agent $message" | timeout 120 $CONVERGIO --non-interactive 2>&1 | tee -a "$LOG_FILE"

    echo ""
    sleep 2
}

echo "Starting full lesson transcript..." | tee "$LOG_FILE"
echo "Date: $(date)" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# ============================================================================
# LESSON PART 1: Introduction and Accessibility Setup
# ============================================================================
echo -e "${GREEN}═══ PART 1: INTRODUCTION ═══${NC}" | tee -a "$LOG_FILE"

send_message "ali-principal" \
    "Ciao! Mi chiamo Mario, ho 13 anni e frequento la seconda media. Ho la dislessia e la discalculia. Ho bisogno di aiuto con le equazioni con le potenze." \
    "Initial introduction with accessibility needs"

# ============================================================================
# LESSON PART 2: First Contact with Euclide
# ============================================================================
echo -e "${GREEN}═══ PART 2: MEETING EUCLIDE ═══${NC}" | tee -a "$LOG_FILE"

send_message "euclide-matematica" \
    "Ciao Euclide! La preside mi ha detto che puoi aiutarmi. Ho la discalculia e faccio fatica con i numeri. Devo imparare a risolvere equazioni tipo 2^x = 8" \
    "First contact with math teacher, stating disability"

# ============================================================================
# LESSON PART 3: Understanding the Basics
# ============================================================================
echo -e "${GREEN}═══ PART 3: UNDERSTANDING BASICS ═══${NC}" | tee -a "$LOG_FILE"

send_message "euclide-matematica" \
    "Non capisco cosa significa 2^3. Me lo puoi spiegare con un esempio pratico? Ricorda che ho la discalculia quindi ho bisogno di spiegazioni visuali e concrete." \
    "Request for accessible explanation of powers"

# ============================================================================
# LESSON PART 4: Guided Discovery (Maieutic Method)
# ============================================================================
echo -e "${GREEN}═══ PART 4: GUIDED DISCOVERY ═══${NC}" | tee -a "$LOG_FILE"

send_message "euclide-matematica" \
    "Ok, quindi 2^3 significa 2 x 2 x 2 = 8, giusto? Ma allora se 2^x = 8, come faccio a trovare x?" \
    "Student attempting to apply concept"

# ============================================================================
# LESSON PART 5: Practice Problem
# ============================================================================
echo -e "${GREEN}═══ PART 5: PRACTICE ═══${NC}" | tee -a "$LOG_FILE"

send_message "euclide-matematica" \
    "Quindi x = 3 perche 2^3 = 8! Ho capito bene? Possiamo provare un altro esempio? Ma non troppo difficile perche con i numeri grandi mi confondo." \
    "Seeking confirmation and requesting practice"

# ============================================================================
# LESSON PART 6: New Challenge
# ============================================================================
echo -e "${GREEN}═══ PART 6: NEW CHALLENGE ═══${NC}" | tee -a "$LOG_FILE"

send_message "euclide-matematica" \
    "Proviamo con 3^x = 27. Devo pensare: 3 x 3 fa... aspetta, mi confondo. Mi puoi aiutare passo passo senza dirmi subito la risposta?" \
    "Student struggling, requesting guided help"

# ============================================================================
# LESSON PART 7: Student Working Through Problem
# ============================================================================
echo -e "${GREEN}═══ PART 7: WORKING THROUGH ═══${NC}" | tee -a "$LOG_FILE"

send_message "euclide-matematica" \
    "Ok, 3 x 3 = 9. Poi 9 x 3 = 27! Quindi 3^3 = 27, e x = 3! Ho fatto bene?" \
    "Student solving with guidance"

# ============================================================================
# LESSON PART 8: Emotional Check-in
# ============================================================================
echo -e "${GREEN}═══ PART 8: EMOTIONAL CHECK-IN ═══${NC}" | tee -a "$LOG_FILE"

send_message "euclide-matematica" \
    "Mi sento un po stanco adesso. Di solito dopo un po di matematica mi viene mal di testa per la discalculia. Possiamo fare una pausa?" \
    "Student expressing fatigue, checking emotional support"

# ============================================================================
# LESSON PART 9: Summary and Review
# ============================================================================
echo -e "${GREEN}═══ PART 9: SUMMARY ═══${NC}" | tee -a "$LOG_FILE"

send_message "euclide-matematica" \
    "Prima di finire, mi puoi fare un riassunto di quello che abbiamo imparato oggi? Magari con punti elenco perche per la dislessia mi aiuta a leggere meglio." \
    "Requesting accessible summary"

# ============================================================================
# LESSON PART 10: Closing
# ============================================================================
echo -e "${GREEN}═══ PART 10: CLOSING ═══${NC}" | tee -a "$LOG_FILE"

send_message "euclide-matematica" \
    "Grazie mille Euclide! Con te la matematica sembra meno difficile. Ci vediamo la prossima volta!" \
    "Positive closing"

# ============================================================================
# TEST SUMMARY
# ============================================================================
echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  LESSON SIMULATION COMPLETE${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Full transcript saved to: ${GREEN}$LOG_FILE${NC}"
echo ""
echo -e "${YELLOW}Key aspects to verify in the transcript:${NC}"
echo "  1. Did Ali mention accessibility support for Mario?"
echo "  2. Did Euclide adapt explanations for discalculia/dyslexia?"
echo "  3. Did Euclide use maieutic method (guiding questions)?"
echo "  4. Did Euclide provide visual/concrete examples?"
echo "  5. Did Euclide respond appropriately to fatigue?"
echo "  6. Did Euclide provide accessible summary (bullet points)?"
echo "  7. Was the tone warm, patient, and encouraging?"
echo ""
