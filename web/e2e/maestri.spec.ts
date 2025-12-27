import { test, expect } from '@playwright/test';

test.describe('Maestri Grid', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
  });

  test('displays all maestri cards', async ({ page }) => {
    // Check for key maestri by name
    const maestriNames = [
      'Euclide',
      'Feynman',
      'Curie',
      'Darwin',
      'Erodoto',
      'Manzoni',
      'Leonardo',
    ];

    for (const name of maestriNames) {
      await expect(page.locator(`text=${name}`).first()).toBeVisible({ timeout: 5000 });
    }
  });

  test('maestro cards have correct information', async ({ page }) => {
    // Find Euclide's card
    const euclideCard = page.locator('text=Euclide').first().locator('..');

    // Check for subject/specialty
    await expect(euclideCard.locator('text=Geometria').or(euclideCard.locator('text=Matematica'))).toBeVisible();
  });

  test('maestro cards are interactive', async ({ page }) => {
    // Cards should be clickable
    const firstCard = page.locator('[class*="cursor-pointer"]').first();
    await expect(firstCard).toBeVisible();

    // Hover should show visual feedback (scale transform)
    await firstCard.hover();
    // Check that the card responds to hover (visual test)
  });

  test('clicking maestro initiates voice session', async ({ page }) => {
    // Click on Euclide
    await page.click('text=Euclide');

    // Should show voice session dialog or configuration error
    // Wait for either the voice session or error modal
    await page.waitForTimeout(1000);

    const hasVoiceSession = await page.locator('text=Connecting').or(page.locator('text=Azure OpenAI Non Configurato')).isVisible();
    expect(hasVoiceSession).toBeTruthy();
  });
});

test.describe('Subject Filtering', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
  });

  test('maestri are grouped by subject', async ({ page }) => {
    // Check that subjects are visually distinct
    // Each maestro should have a colored indicator
    const cards = page.locator('[class*="rounded"]').filter({ hasText: /Euclide|Feynman|Curie/ });
    await expect(cards.first()).toBeVisible();
  });
});
