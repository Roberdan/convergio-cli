'use client';

import { ThemeProvider } from 'next-themes';
import { AccessibilityProvider } from '@/components/accessibility';

interface ProvidersProps {
  children: React.ReactNode;
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
        {children}
      </AccessibilityProvider>
    </ThemeProvider>
  );
}
