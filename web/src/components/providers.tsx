'use client';

import { AccessibilityProvider } from '@/components/accessibility';

interface ProvidersProps {
  children: React.ReactNode;
}

export function Providers({ children }: ProvidersProps) {
  return (
    <AccessibilityProvider>
      {children}
    </AccessibilityProvider>
  );
}
