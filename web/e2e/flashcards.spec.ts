import { test, expect } from '@playwright/test';

test.describe('Flashcards View', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.click('text=Flashcards');
  });

  test('flashcards page loads correctly', async ({ page }) => {
    // Check for flashcards header or content
    await expect(page.locator('text=Flashcard').first()).toBeVisible();
  });

  test('displays flashcard decks or empty state', async ({ page }) => {
    // Should show either existing decks or empty/coming soon message
    const hasContent = await page.locator('[class*="card"]').or(page.locator('text=presto')).isVisible();
    expect(hasContent).toBeTruthy();
  });
});

test.describe('Homework Help View', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.click('text=Compiti');
  });

  test('homework help page loads', async ({ page }) => {
    // Check for homework-related content
    await expect(page.locator('text=Compiti').or(page.locator('text=aiuto'))).toBeVisible();
  });
});

test.describe('Progress View', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.click('text=Progressi');
  });

  test('progress page loads correctly', async ({ page }) => {
    // Check for progress-related content
    await expect(page.locator('text=Progresso').or(page.locator('text=XP').or(page.locator('text=Level')))).toBeVisible();
  });

  test('displays XP information', async ({ page }) => {
    // Should show XP somewhere
    const hasXP = await page.locator('text=XP').isVisible();
    expect(hasXP).toBeTruthy();
  });

  test('displays streak information', async ({ page }) => {
    // Should show streak information
    const hasStreak = await page.locator('text=Streak').or(page.locator('text=streak')).or(page.locator('text=giorni')).isVisible();
    expect(hasStreak).toBeTruthy();
  });
});
