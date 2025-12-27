import { test, expect } from '@playwright/test';

test.describe('Settings View', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.click('text=Impostazioni');
  });

  test('settings page loads correctly', async ({ page }) => {
    // Check for settings header
    await expect(page.locator('h1').filter({ hasText: 'Impostazioni' })).toBeVisible();

    // Check for tabs
    await expect(page.locator('text=Profilo')).toBeVisible();
    await expect(page.locator('text=Accessibilita')).toBeVisible();
    await expect(page.locator('text=Aspetto')).toBeVisible();
  });

  test('profile tab allows name input', async ({ page }) => {
    // Profile tab should be active by default
    await expect(page.locator('text=Informazioni Personali')).toBeVisible();

    // Should have name input
    const nameInput = page.locator('input[placeholder*="chiami"]');
    await expect(nameInput).toBeVisible();

    // Should be able to type name
    await nameInput.fill('Test Student');
    await expect(nameInput).toHaveValue('Test Student');
  });

  test('grade level selector works', async ({ page }) => {
    // Find grade level select
    const gradeSelect = page.locator('select');
    await expect(gradeSelect.first()).toBeVisible();

    // Select an option
    await gradeSelect.first().selectOption('primary');
    await expect(gradeSelect.first()).toHaveValue('primary');
  });

  test('learning goals can be selected', async ({ page }) => {
    // Check for learning goals
    await expect(page.locator('text=Obiettivi di Apprendimento')).toBeVisible();

    // Click on a goal
    await page.click('text=Migliorare in matematica');

    // Should be selected (check for visual change)
    const goal = page.locator('text=Migliorare in matematica').locator('..');
    await expect(goal).toHaveClass(/border-blue|bg-blue/);
  });
});

test.describe('Theme Settings', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.click('text=Impostazioni');
    await page.click('text=Aspetto');
  });

  test('theme options are displayed', async ({ page }) => {
    // Should show theme options
    await expect(page.locator('text=Chiaro')).toBeVisible();
    await expect(page.locator('text=Scuro')).toBeVisible();
    await expect(page.locator('text=Sistema')).toBeVisible();
  });

  test('can switch to light theme', async ({ page }) => {
    // Click on light theme
    await page.click('text=Chiaro');

    // Body should not have dark class
    const html = page.locator('html');
    await expect(html).not.toHaveClass(/dark/);
  });

  test('can switch to dark theme', async ({ page }) => {
    // Click on dark theme
    await page.click('text=Scuro');

    // Wait for theme change
    await page.waitForTimeout(500);

    // HTML should have dark class
    const html = page.locator('html');
    await expect(html).toHaveClass(/dark/);
  });

  test('accent color options are displayed', async ({ page }) => {
    // Should show color options
    await expect(page.locator('text=Colore Principale')).toBeVisible();

    // Should have color buttons
    const colorButtons = page.locator('[class*="rounded-full"][class*="bg-"]').filter({ hasNotText: /./ });
    await expect(colorButtons.first()).toBeVisible();
  });
});

test.describe('Accessibility Settings', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.click('text=Impostazioni');
    await page.click('button:has-text("Accessibilita")');
  });

  test('accessibility tab shows options', async ({ page }) => {
    // Check for accessibility info
    await expect(page.locator('text=Accessibilita')).toBeVisible();

    // Check for dyslexia support mention
    await expect(page.locator('text=Dislessia').or(page.locator('text=dislessia'))).toBeVisible();
  });

  test('can open accessibility modal', async ({ page }) => {
    // Click to open full accessibility panel
    await page.click('text=Apri Pannello Accessibilita Completo');

    // Modal should appear
    await expect(page.locator('[role="dialog"]').or(page.locator('[class*="modal"]').or(page.locator('[class*="fixed"]').filter({ hasText: 'Font' })))).toBeVisible();
  });
});

test.describe('Privacy Settings', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.click('text=Impostazioni');
    await page.click('button:has-text("Privacy")');
  });

  test('privacy info is displayed', async ({ page }) => {
    // Check for privacy information
    await expect(page.locator('text=Privacy')).toBeVisible();

    // Check for data protection info
    await expect(page.locator('text=dati').first()).toBeVisible();
  });

  test('delete data button exists', async ({ page }) => {
    // Should have delete data option
    await expect(page.locator('text=Elimina tutti i miei dati')).toBeVisible();
  });
});
