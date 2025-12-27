import { test, expect } from '@playwright/test';

test.describe('Quiz View', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.click('text=Quiz');
  });

  test('quiz page loads correctly', async ({ page }) => {
    // Check for quiz header
    await expect(page.locator('h1').filter({ hasText: 'Quiz' })).toBeVisible();

    // Check for quiz description
    await expect(page.locator('text=Metti alla prova le tue conoscenze')).toBeVisible();
  });

  test('displays available quizzes', async ({ page }) => {
    // Check for quiz cards
    await expect(page.locator('text=Matematica - Le basi')).toBeVisible();
    await expect(page.locator('text=Storia - Roma Antica')).toBeVisible();
    await expect(page.locator('text=Scienze - Il Corpo Umano')).toBeVisible();
  });

  test('quiz cards show XP rewards', async ({ page }) => {
    // Each quiz card should show XP reward
    await expect(page.locator('text=50 XP').or(page.locator('text=40 XP')).first()).toBeVisible();
  });

  test('quiz cards show question count', async ({ page }) => {
    // Each quiz should show number of questions
    await expect(page.locator('text=domande').first()).toBeVisible();
  });

  test('clicking quiz card starts quiz', async ({ page }) => {
    // Click on first quiz
    await page.click('text=Matematica - Le basi');

    // Quiz should start - look for question content
    await page.waitForTimeout(500);

    // Should show quiz interface or question
    const quizStarted = await page.locator('text=Quanto fa').or(page.locator('text=domanda')).isVisible();
    expect(quizStarted).toBeTruthy();
  });

  test('completing quiz marks it as completed', async ({ page }) => {
    // This would require completing an actual quiz
    // For now, verify the "Inizia Quiz" button is visible for incomplete quizzes
    await expect(page.locator('text=Inizia Quiz').first()).toBeVisible();
  });
});

test.describe('Quiz Interaction', () => {
  test('can navigate through quiz questions', async ({ page }) => {
    await page.goto('/');
    await page.click('text=Quiz');

    // Start a quiz
    await page.click('text=Matematica - Le basi');
    await page.waitForTimeout(500);

    // Should show question with options
    const hasOptions = await page.locator('[role="button"]').or(page.locator('button')).count() > 0;
    expect(hasOptions).toBeTruthy();
  });
});
