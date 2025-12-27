'use client';

import { useState } from 'react';
import { motion } from 'framer-motion';
import {
  GraduationCap,
  BookOpen,
  Brain,
  Trophy,
  Settings,
  Menu,
  X,
  Sparkles,
  TrendingUp,
  Target,
  Flame,
} from 'lucide-react';
import { MaestriGrid } from '@/components/maestros/maestri-grid';
import { FlashcardsView, HomeworkHelpView } from '@/components/education';
import { SettingsView } from '@/components/settings';
import { ProgressView } from '@/components/progress';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { useProgressStore, useSettingsStore } from '@/lib/stores/app-store';
import { cn } from '@/lib/utils';

type View = 'maestri' | 'quiz' | 'flashcards' | 'homework' | 'progress' | 'settings';

export default function Home() {
  const [currentView, setCurrentView] = useState<View>('maestri');
  const [sidebarOpen, setSidebarOpen] = useState(true);

  const { xp, level, streak, totalStudyMinutes } = useProgressStore();
  const { studentProfile } = useSettingsStore();

  const navItems = [
    { id: 'maestri' as const, label: 'Maestri', icon: GraduationCap },
    { id: 'quiz' as const, label: 'Quiz', icon: Brain },
    { id: 'flashcards' as const, label: 'Flashcards', icon: BookOpen },
    { id: 'homework' as const, label: 'Compiti', icon: Target },
    { id: 'progress' as const, label: 'Progressi', icon: Trophy },
    { id: 'settings' as const, label: 'Impostazioni', icon: Settings },
  ];

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-50 to-slate-100 dark:from-slate-900 dark:to-slate-950">
      {/* Sidebar */}
      <aside
        className={cn(
          'fixed top-0 left-0 h-full bg-white dark:bg-slate-900 border-r border-slate-200 dark:border-slate-800 z-40 transition-all duration-300',
          sidebarOpen ? 'w-64' : 'w-20'
        )}
      >
        {/* Logo */}
        <div className="h-16 flex items-center justify-between px-4 border-b border-slate-200 dark:border-slate-800">
          <motion.div
            className="flex items-center gap-3"
            animate={{ opacity: sidebarOpen ? 1 : 0 }}
          >
            <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-blue-500 to-indigo-600 flex items-center justify-center">
              <Sparkles className="h-5 w-5 text-white" />
            </div>
            {sidebarOpen && (
              <span className="font-bold text-lg text-slate-900 dark:text-white">
                Convergio
              </span>
            )}
          </motion.div>
          <Button
            variant="ghost"
            size="icon-sm"
            onClick={() => setSidebarOpen(!sidebarOpen)}
            className="text-slate-500"
          >
            {sidebarOpen ? <X className="h-4 w-4" /> : <Menu className="h-4 w-4" />}
          </Button>
        </div>

        {/* Quick stats */}
        {sidebarOpen && (
          <div className="p-4 border-b border-slate-200 dark:border-slate-800">
            <div className="grid grid-cols-2 gap-3">
              <div className="p-3 rounded-xl bg-gradient-to-br from-amber-50 to-orange-50 dark:from-amber-900/20 dark:to-orange-900/20">
                <Flame className="h-5 w-5 text-orange-500 mb-1" />
                <p className="text-2xl font-bold text-orange-600">{streak.current}</p>
                <p className="text-xs text-orange-600/80">Streak</p>
              </div>
              <div className="p-3 rounded-xl bg-gradient-to-br from-blue-50 to-indigo-50 dark:from-blue-900/20 dark:to-indigo-900/20">
                <TrendingUp className="h-5 w-5 text-blue-500 mb-1" />
                <p className="text-2xl font-bold text-blue-600">{level}</p>
                <p className="text-xs text-blue-600/80">Level</p>
              </div>
            </div>
          </div>
        )}

        {/* Navigation */}
        <nav className="p-4 space-y-2">
          {navItems.map((item) => (
            <button
              key={item.id}
              onClick={() => setCurrentView(item.id)}
              className={cn(
                'w-full flex items-center gap-3 px-4 py-3 rounded-xl transition-all',
                currentView === item.id
                  ? 'bg-blue-500 text-white shadow-lg shadow-blue-500/30'
                  : 'text-slate-600 dark:text-slate-400 hover:bg-slate-100 dark:hover:bg-slate-800'
              )}
            >
              <item.icon className="h-5 w-5 flex-shrink-0" />
              {sidebarOpen && <span className="font-medium">{item.label}</span>}
            </button>
          ))}
        </nav>

        {/* XP Progress */}
        {sidebarOpen && (
          <div className="absolute bottom-0 left-0 right-0 p-4 border-t border-slate-200 dark:border-slate-800">
            <div className="mb-2 flex justify-between text-sm">
              <span className="text-slate-600 dark:text-slate-400">XP</span>
              <span className="font-medium text-slate-900 dark:text-white">
                {xp.toLocaleString()}
              </span>
            </div>
            <div className="h-2 rounded-full bg-slate-200 dark:bg-slate-700 overflow-hidden">
              <motion.div
                className="h-full bg-gradient-to-r from-blue-500 to-indigo-500"
                initial={{ width: 0 }}
                animate={{ width: `${(xp % 1000) / 10}%` }}
              />
            </div>
          </div>
        )}
      </aside>

      {/* Main content */}
      <main
        className={cn(
          'min-h-screen transition-all duration-300 p-8',
          sidebarOpen ? 'ml-64' : 'ml-20'
        )}
      >
        {/* Welcome header */}
        {studentProfile.name && (
          <div className="mb-8">
            <h2 className="text-2xl font-bold text-slate-900 dark:text-white">
              Ciao, {studentProfile.name}! 👋
            </h2>
            <p className="text-slate-600 dark:text-slate-400">
              Pronto per una nuova sessione di studio?
            </p>
          </div>
        )}

        {/* View content */}
        <motion.div
          key={currentView}
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.3 }}
        >
          {currentView === 'maestri' && <MaestriGrid />}

          {currentView === 'quiz' && (
            <div className="text-center py-20">
              <Brain className="h-16 w-16 mx-auto text-slate-400 mb-4" />
              <h2 className="text-2xl font-bold text-slate-900 dark:text-white mb-2">
                Quiz in arrivo
              </h2>
              <p className="text-slate-600 dark:text-slate-400">
                Questa sezione sarà presto disponibile
              </p>
            </div>
          )}

          {currentView === 'flashcards' && <FlashcardsView />}

          {currentView === 'homework' && <HomeworkHelpView />}

          {currentView === 'progress' && <ProgressView />}

          {currentView === 'settings' && <SettingsView />}
        </motion.div>
      </main>
    </div>
  );
}
