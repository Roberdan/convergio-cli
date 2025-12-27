import { test, expect } from '@playwright/test';

test.describe('Navigation', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
  });

  test('homepage loads with maestri grid', async ({ page }) => {
    // Check title or main heading
    await expect(page.locator('text=Convergio-Edu')).toBeVisible();

    // Check maestri are displayed
    const maestriCards = page.locator('[class*="maestro"]').or(page.locator('text=Euclide'));
    await expect(maestriCards.first()).toBeVisible({ timeout: 10000 });
  });

  test('sidebar navigation works', async ({ page }) => {
    // Click on Quiz nav item
    await page.click('text=Quiz');
    await expect(page.locator('text=Metti alla prova le tue conoscenze')).toBeVisible();

    // Click on Flashcards nav item
    await page.click('text=Flashcards');
    await expect(page.locator('text=Flashcard')).toBeVisible();

    // Click on Progressi nav item
    await page.click('text=Progressi');
    await expect(page.locator('text=Progresso')).toBeVisible();

    // Click on Impostazioni nav item
    await page.click('text=Impostazioni');
    await expect(page.locator('text=Personalizza')).toBeVisible();
  });

  test('sidebar collapses and expands', async ({ page }) => {
    // Find the collapse button (X icon when open)
    const collapseButton = page.locator('aside button').first();

    // Should start open
    await expect(page.locator('aside').locator('text=Convergio-Edu')).toBeVisible();

    // Click to collapse
    await collapseButton.click();

    // Text should be hidden in collapsed state
    await expect(page.locator('aside')).toHaveClass(/w-20/);

    // Click to expand again
    await collapseButton.click();
    await expect(page.locator('aside')).toHaveClass(/w-64/);
  });
});

test.describe('Responsive Design', () => {
  test('mobile layout works', async ({ page }) => {
    await page.setViewportSize({ width: 375, height: 667 });
    await page.goto('/');

    // Page should still load
    await expect(page.locator('aside')).toBeVisible();
  });

  test('tablet layout works', async ({ page }) => {
    await page.setViewportSize({ width: 768, height: 1024 });
    await page.goto('/');

    // Page should display properly
    await expect(page.locator('aside')).toBeVisible();
  });
});
