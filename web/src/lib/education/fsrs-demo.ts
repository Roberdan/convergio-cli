/**
 * FSRS Demo - Quick demonstration of the algorithm
 *
 * Run this file to see FSRS in action:
 * npx ts-node src/lib/education/fsrs-demo.ts
 */

import {
  createCard,
  reviewCard,
  calculateRetrievability,
  isDue,
  calculateStats,
  getDueCards,
  predictRetention,
  type FSRSCard,
} from './fsrs.js';

console.log('='.repeat(60));
console.log('FSRS Algorithm Demonstration');
console.log('='.repeat(60));

// Create a new card
console.log('\n1. Creating a new flashcard...');
let card = createCard();
console.log('Initial state:', {
  stability: card.stability.toFixed(2),
  difficulty: card.difficulty.toFixed(2),
  reps: card.reps,
  lapses: card.lapses,
});

// First review - Good
console.log('\n2. First review (Quality: Good)...');
card = reviewCard(card, 3);
console.log('After review:', {
  stability: card.stability.toFixed(2),
  difficulty: card.difficulty.toFixed(2),
  retrievability: (calculateRetrievability(card) * 100).toFixed(1) + '%',
  nextReview: card.nextReview.toLocaleString(),
  reps: card.reps,
});

// Second review - Easy
console.log('\n3. Second review (Quality: Easy)...');
card = reviewCard(card, 4);
console.log('After review:', {
  stability: card.stability.toFixed(2),
  difficulty: card.difficulty.toFixed(2),
  nextReview: card.nextReview.toLocaleString(),
  reps: card.reps,
});

// Simulate forgetting
console.log('\n4. Simulating forgetting (Quality: Again)...');
card = reviewCard(card, 1);
console.log('After forgetting:', {
  stability: card.stability.toFixed(2),
  difficulty: card.difficulty.toFixed(2),
  lapses: card.lapses,
  nextReview: card.nextReview.toLocaleString(),
});

// Recovery
console.log('\n5. Recovery review (Quality: Good)...');
card = reviewCard(card, 3);
console.log('After recovery:', {
  stability: card.stability.toFixed(2),
  difficulty: card.difficulty.toFixed(2),
  reps: card.reps,
  lapses: card.lapses,
});

// Predict retention
console.log('\n6. Retention predictions:');
const in1Day = new Date(Date.now() + 1 * 24 * 60 * 60 * 1000);
const in7Days = new Date(Date.now() + 7 * 24 * 60 * 60 * 1000);
const in30Days = new Date(Date.now() + 30 * 24 * 60 * 60 * 1000);

console.log(`  In 1 day:  ${(predictRetention(card, in1Day) * 100).toFixed(1)}%`);
console.log(`  In 7 days: ${(predictRetention(card, in7Days) * 100).toFixed(1)}%`);
console.log(`  In 30 days: ${(predictRetention(card, in30Days) * 100).toFixed(1)}%`);

// Deck statistics
console.log('\n7. Creating a deck of cards...');
const deck: FSRSCard[] = [];

// Add 10 cards with varied histories
for (let i = 0; i < 10; i++) {
  let deckCard = createCard();

  // Simulate different learning patterns
  if (i < 3) {
    // Easy cards - all good/easy reviews
    deckCard = reviewCard(deckCard, 4);
    deckCard = reviewCard(deckCard, 4);
    deckCard = reviewCard(deckCard, 3);
  } else if (i < 6) {
    // Medium cards - mixed reviews
    deckCard = reviewCard(deckCard, 3);
    deckCard = reviewCard(deckCard, 2);
    deckCard = reviewCard(deckCard, 3);
  } else {
    // Hard cards - with lapses
    deckCard = reviewCard(deckCard, 1);
    deckCard = reviewCard(deckCard, 2);
    deckCard = reviewCard(deckCard, 1);
  }

  deck.push(deckCard);
}

const stats = calculateStats(deck);
console.log('Deck statistics:', {
  totalCards: stats.totalCards,
  cardsDue: stats.cardsDue,
  cardsMastered: stats.cardsMastered,
  avgStability: stats.avgStability.toFixed(2) + ' days',
  avgDifficulty: (stats.avgDifficulty * 100).toFixed(1) + '%',
  predictedRetention: (stats.predictedRetention * 100).toFixed(1) + '%',
});

// Get due cards
console.log('\n8. Getting due cards...');
const dueCards = getDueCards(deck, 5);
console.log(`Found ${dueCards.length} cards due for review (limit: 5)`);

// Learning curve visualization
console.log('\n9. Learning curve for a single card:');
console.log('Reviews | Stability | Difficulty | Quality');
console.log('-'.repeat(50));

let learningCard = createCard();
const reviewPattern = [3, 3, 4, 3, 2, 3, 4, 4]; // Realistic pattern

console.log(`   ${learningCard.reps.toString().padStart(4)}   | ${learningCard.stability.toFixed(2).padStart(9)} | ${(learningCard.difficulty * 100).toFixed(0).padStart(10)}% |    -`);

reviewPattern.forEach((quality, index) => {
  learningCard = reviewCard(learningCard, quality as 1 | 2 | 3 | 4);
  const qualityName = ['', 'Again', 'Hard', 'Good', 'Easy'][quality];
  console.log(
    `   ${learningCard.reps.toString().padStart(4)}   | ` +
    `${learningCard.stability.toFixed(2).padStart(9)} | ` +
    `${(learningCard.difficulty * 100).toFixed(0).padStart(10)}% | ` +
    `${qualityName}`
  );
});

console.log('\n' + '='.repeat(60));
console.log('Demo complete!');
console.log('='.repeat(60));
console.log('\nKey Observations:');
console.log('- Stability increases with good reviews (longer intervals)');
console.log('- Difficulty decreases when card is easy');
console.log('- Forgetting (quality=1) reduces stability and adds lapses');
console.log('- The algorithm adapts to your actual performance');
console.log('='.repeat(60) + '\n');
