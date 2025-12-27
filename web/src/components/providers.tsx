'use client';

import { useEffect } from 'react';
import { ThemeProvider } from 'next-themes';
import { AccessibilityProvider } from '@/components/accessibility';
import { useSettingsStore } from '@/lib/stores/app-store';

interface ProvidersProps {
  children: React.ReactNode;
}

// Component to apply accent color from settings
function AccentColorApplier() {
  const { appearance } = useSettingsStore();

  useEffect(() => {
    // Apply accent color to document root
    if (appearance?.accentColor) {
      document.documentElement.setAttribute('data-accent', appearance.accentColor);
    }
  }, [appearance?.accentColor]);

  return null;
}

export function Providers({ children }: ProvidersProps) {
  return (
    <ThemeProvider
      attribute="class"
      defaultTheme="system"
      enableSystem
      disableTransitionOnChange
    >
      <AccessibilityProvider>
        <AccentColorApplier />
        {children}
      </AccessibilityProvider>
    </ThemeProvider>
  );
}
