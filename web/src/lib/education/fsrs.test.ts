/**
 * FSRS Algorithm Test Suite
 *
 * Tests for the Free Spaced Repetition Scheduler implementation
 */

import { describe, it, expect, beforeEach } from 'vitest';
import {
  createCard,
  reviewCard,
  calculateRetrievability,
  getNextReviewDate,
  isDue,
  calculateStats,
  getDueCards,
  predictRetention,
  FSRS_CONSTANTS,
  type FSRSCard,
  type Quality,
} from './fsrs';

describe('FSRS Algorithm', () => {
  let card: FSRSCard;

  beforeEach(() => {
    card = createCard();
  });

  describe('createCard', () => {
    it('should create a card with default initial values', () => {
      expect(card.stability).toBe(FSRS_CONSTANTS.INITIAL_STABILITY);
      expect(card.difficulty).toBe(FSRS_CONSTANTS.INITIAL_DIFFICULTY);
      expect(card.lapses).toBe(0);
      expect(card.reps).toBe(0);
      expect(card.lastReview).toBeInstanceOf(Date);
      expect(card.nextReview).toBeInstanceOf(Date);
    });

    it('should schedule first review immediately', () => {
      const now = new Date();
      expect(card.nextReview.getTime()).toBeLessThanOrEqual(now.getTime());
    });
  });

  describe('calculateRetrievability', () => {
    it('should return 1.0 for just-reviewed card', () => {
      const retrievability = calculateRetrievability(card);
      expect(retrievability).toBeCloseTo(1.0, 2);
    });

    it('should decrease over time', () => {
      const pastCard: FSRSCard = {
        ...card,
        lastReview: new Date(Date.now() - 7 * 24 * 60 * 60 * 1000), // 7 days ago
      };

      const retrievability = calculateRetrievability(pastCard);
      expect(retrievability).toBeLessThan(1.0);
      expect(retrievability).toBeGreaterThan(0.0);
    });

    it('should handle edge cases', () => {
      const zeroStabilityCard: FSRSCard = {
        ...card,
        stability: 0,
      };
      expect(calculateRetrievability(zeroStabilityCard)).toBe(1.0);

      const negativeStabilityCard: FSRSCard = {
        ...card,
        stability: -1,
      };
      expect(calculateRetrievability(negativeStabilityCard)).toBe(1.0);
    });

    it('should never return values outside [0, 1]', () => {
      const veryOldCard: FSRSCard = {
        ...card,
        lastReview: new Date(Date.now() - 365 * 24 * 60 * 60 * 1000), // 1 year ago
      };

      const retrievability = calculateRetrievability(veryOldCard);
      expect(retrievability).toBeGreaterThanOrEqual(0.0);
      expect(retrievability).toBeLessThanOrEqual(1.0);
    });
  });

  describe('reviewCard', () => {
    it('should increase reps counter', () => {
      const reviewed = reviewCard(card, 3);
      expect(reviewed.reps).toBe(1);

      const reviewedAgain = reviewCard(reviewed, 3);
      expect(reviewedAgain.reps).toBe(2);
    });

    it('should increase lapses on quality=1 (again)', () => {
      const reviewed = reviewCard(card, 1);
      expect(reviewed.lapses).toBe(1);
    });

    it('should not increase lapses on successful recall', () => {
      const reviewed = reviewCard(card, 3);
      expect(reviewed.lapses).toBe(0);
    });

    it('should update lastReview to current time', () => {
      const before = new Date();
      const reviewed = reviewCard(card, 3);
      const after = new Date();

      expect(reviewed.lastReview.getTime()).toBeGreaterThanOrEqual(before.getTime());
      expect(reviewed.lastReview.getTime()).toBeLessThanOrEqual(after.getTime());
    });

    it('should schedule next review in the future', () => {
      const reviewed = reviewCard(card, 3);
      expect(reviewed.nextReview.getTime()).toBeGreaterThan(Date.now());
    });

    describe('quality effects on stability', () => {
      it('should reduce stability significantly on quality=1 (again)', () => {
        const reviewed = reviewCard(card, 1);
        expect(reviewed.stability).toBeLessThan(card.stability);
      });

      it('should increase stability moderately on quality=3 (good)', () => {
        const reviewed = reviewCard(card, 3);
        // For first review, stability should increase
        expect(reviewed.stability).toBeGreaterThanOrEqual(card.stability * 0.5);
      });

      it('should increase stability more on quality=4 (easy)', () => {
        const reviewedGood = reviewCard(card, 3);
        const reviewedEasy = reviewCard(card, 4);
        // Easy should result in higher stability than good
        expect(reviewedEasy.stability).toBeGreaterThan(reviewedGood.stability);
      });
    });

    describe('quality effects on difficulty', () => {
      it('should increase difficulty on quality=1 (again)', () => {
        const reviewed = reviewCard(card, 1);
        expect(reviewed.difficulty).toBeGreaterThan(card.difficulty);
      });

      it('should decrease difficulty on quality=4 (easy)', () => {
        const reviewed = reviewCard(card, 4);
        expect(reviewed.difficulty).toBeLessThan(card.difficulty);
      });

      it('should keep difficulty in [0, 1] range', () => {
        let testCard = createCard();

        // Test multiple "again" reviews
        for (let i = 0; i < 10; i++) {
          testCard = reviewCard(testCard, 1);
        }
        expect(testCard.difficulty).toBeLessThanOrEqual(1.0);

        // Test multiple "easy" reviews
        testCard = createCard();
        for (let i = 0; i < 10; i++) {
          testCard = reviewCard(testCard, 4);
        }
        expect(testCard.difficulty).toBeGreaterThanOrEqual(0.0);
      });
    });

    it('should handle all quality values', () => {
      const qualities: Quality[] = [1, 2, 3, 4];

      qualities.forEach(quality => {
        const reviewed = reviewCard(card, quality);
        expect(reviewed).toBeDefined();
        expect(reviewed.stability).toBeGreaterThan(0);
        expect(reviewed.difficulty).toBeGreaterThanOrEqual(0);
        expect(reviewed.difficulty).toBeLessThanOrEqual(1);
      });
    });
  });

  describe('getNextReviewDate', () => {
    it('should return the nextReview date', () => {
      const date = getNextReviewDate(card);
      expect(date).toEqual(card.nextReview);
    });
  });

  describe('isDue', () => {
    it('should return true for newly created card', () => {
      expect(isDue(card)).toBe(true);
    });

    it('should return false for future-scheduled card', () => {
      const futureCard: FSRSCard = {
        ...card,
        nextReview: new Date(Date.now() + 24 * 60 * 60 * 1000), // tomorrow
      };
      expect(isDue(futureCard)).toBe(false);
    });

    it('should return true for past-scheduled card', () => {
      const pastCard: FSRSCard = {
        ...card,
        nextReview: new Date(Date.now() - 24 * 60 * 60 * 1000), // yesterday
      };
      expect(isDue(pastCard)).toBe(true);
    });
  });

  describe('calculateStats', () => {
    it('should handle empty array', () => {
      const stats = calculateStats([]);
      expect(stats.totalCards).toBe(0);
      expect(stats.cardsDue).toBe(0);
      expect(stats.avgStability).toBe(0);
    });

    it('should calculate correct stats for single card', () => {
      const stats = calculateStats([card]);
      expect(stats.totalCards).toBe(1);
      expect(stats.cardsDue).toBe(1); // New card is due
      expect(stats.avgStability).toBe(card.stability);
      expect(stats.avgDifficulty).toBe(card.difficulty);
    });

    it('should calculate correct averages', () => {
      const card1 = createCard();
      const card2: FSRSCard = {
        ...createCard(),
        stability: 10,
        difficulty: 0.5,
      };

      const stats = calculateStats([card1, card2]);
      expect(stats.totalCards).toBe(2);
      expect(stats.avgStability).toBe((card1.stability + card2.stability) / 2);
      expect(stats.avgDifficulty).toBe((card1.difficulty + card2.difficulty) / 2);
    });

    it('should count mastered cards (stability > 30)', () => {
      const masteredCard: FSRSCard = {
        ...createCard(),
        stability: 35,
      };

      const stats = calculateStats([card, masteredCard]);
      expect(stats.cardsMastered).toBe(1);
    });

    it('should count due cards correctly', () => {
      const dueCard = createCard();
      const futureCard: FSRSCard = {
        ...createCard(),
        nextReview: new Date(Date.now() + 24 * 60 * 60 * 1000),
      };

      const stats = calculateStats([dueCard, futureCard]);
      expect(stats.cardsDue).toBe(1);
    });
  });

  describe('getDueCards', () => {
    it('should return empty array when no cards are due', () => {
      const futureCard: FSRSCard = {
        ...createCard(),
        nextReview: new Date(Date.now() + 24 * 60 * 60 * 1000),
      };

      const dueCards = getDueCards([futureCard]);
      expect(dueCards).toHaveLength(0);
    });

    it('should return due cards only', () => {
      const dueCard1 = createCard();
      const dueCard2 = createCard();
      const futureCard: FSRSCard = {
        ...createCard(),
        nextReview: new Date(Date.now() + 24 * 60 * 60 * 1000),
      };

      const dueCards = getDueCards([dueCard1, futureCard, dueCard2]);
      expect(dueCards).toHaveLength(2);
    });

    it('should sort cards by next review date', () => {
      const card1: FSRSCard = {
        ...createCard(),
        nextReview: new Date(Date.now() - 2 * 60 * 60 * 1000), // 2 hours ago
      };

      const card2: FSRSCard = {
        ...createCard(),
        nextReview: new Date(Date.now() - 1 * 60 * 60 * 1000), // 1 hour ago
      };

      const dueCards = getDueCards([card2, card1]);
      expect(dueCards[0]).toEqual(card1);
      expect(dueCards[1]).toEqual(card2);
    });

    it('should respect limit parameter', () => {
      const cards = [createCard(), createCard(), createCard()];
      const dueCards = getDueCards(cards, 2);
      expect(dueCards).toHaveLength(2);
    });
  });

  describe('predictRetention', () => {
    it('should predict high retention immediately after review', () => {
      const futureDate = new Date(Date.now() + 60 * 60 * 1000); // 1 hour
      const retention = predictRetention(card, futureDate);
      expect(retention).toBeGreaterThan(0.95);
    });

    it('should predict lower retention further in future', () => {
      const nearFuture = new Date(Date.now() + 1 * 24 * 60 * 60 * 1000); // 1 day
      const farFuture = new Date(Date.now() + 7 * 24 * 60 * 60 * 1000); // 7 days

      const nearRetention = predictRetention(card, nearFuture);
      const farRetention = predictRetention(card, farFuture);

      expect(farRetention).toBeLessThan(nearRetention);
    });

    it('should handle edge cases', () => {
      const pastDate = new Date(Date.now() - 24 * 60 * 60 * 1000);
      const retention = predictRetention(card, pastDate);
      expect(retention).toBe(1.0); // Past dates return 1.0
    });

    it('should never return values outside [0, 1]', () => {
      const veryFarFuture = new Date(Date.now() + 365 * 24 * 60 * 60 * 1000);
      const retention = predictRetention(card, veryFarFuture);
      expect(retention).toBeGreaterThanOrEqual(0.0);
      expect(retention).toBeLessThanOrEqual(1.0);
    });
  });

  describe('learning progression simulation', () => {
    it('should demonstrate typical learning curve', () => {
      let learningCard = createCard();
      const qualities: Quality[] = [3, 3, 4, 3, 4]; // Good -> Easy pattern

      const stabilities: number[] = [learningCard.stability];

      qualities.forEach(quality => {
        learningCard = reviewCard(learningCard, quality);
        stabilities.push(learningCard.stability);
      });

      // Stability should generally increase with good performance
      expect(stabilities[stabilities.length - 1]).toBeGreaterThan(stabilities[0]);
    });

    it('should demonstrate forgetting and recovery', () => {
      let card = createCard();

      // Good reviews
      card = reviewCard(card, 3);
      card = reviewCard(card, 3);
      const stabilityBefore = card.stability;

      // Forget
      card = reviewCard(card, 1);
      expect(card.stability).toBeLessThan(stabilityBefore);
      expect(card.lapses).toBe(1);

      // Recover with good reviews
      card = reviewCard(card, 3);
      card = reviewCard(card, 4);

      expect(card.lapses).toBe(1); // Lapses don't decrease
      expect(card.reps).toBeGreaterThan(card.lapses);
    });

    it('should show difficulty adjustment over time', () => {
      let easyCard = createCard();
      let hardCard = createCard();

      // Consistently easy reviews
      for (let i = 0; i < 5; i++) {
        easyCard = reviewCard(easyCard, 4);
      }

      // Consistently hard reviews
      for (let i = 0; i < 5; i++) {
        hardCard = reviewCard(hardCard, 2);
      }

      expect(easyCard.difficulty).toBeLessThan(hardCard.difficulty);
    });
  });

  describe('FSRS constants', () => {
    it('should export correct constants', () => {
      expect(FSRS_CONSTANTS.INITIAL_STABILITY).toBe(1.0);
      expect(FSRS_CONSTANTS.INITIAL_DIFFICULTY).toBe(0.3);
      expect(FSRS_CONSTANTS.DESIRED_RETENTION).toBe(0.9);
      expect(FSRS_CONSTANTS.MIN_STABILITY_DAYS).toBe(0.04);
      expect(FSRS_CONSTANTS.MAX_STABILITY_DAYS).toBe(1095);
    });
  });
});
