'use client';

import { motion } from 'framer-motion';
import { Trophy, Flame, Clock, BookOpen, TrendingUp, Star } from 'lucide-react';
import { Card, CardContent } from '@/components/ui/card';
import { useProgressStore } from '@/lib/stores/app-store';
import { cn } from '@/lib/utils';

const XP_PER_LEVEL = [0, 100, 250, 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000];

export function HomeProgressWidget() {
  const { xp, level, streak, totalStudyMinutes, sessionsThisWeek, questionsAsked } = useProgressStore();

  // Calculate XP progress to next level
  const currentLevelXP = XP_PER_LEVEL[level - 1] || 0;
  const nextLevelXP = XP_PER_LEVEL[level] || XP_PER_LEVEL[XP_PER_LEVEL.length - 1];
  const xpInLevel = xp - currentLevelXP;
  const xpNeeded = nextLevelXP - currentLevelXP;
  const progressPercent = Math.min(100, (xpInLevel / xpNeeded) * 100);

  // Format study time
  const hours = Math.floor(totalStudyMinutes / 60);
  const minutes = totalStudyMinutes % 60;
  const studyTimeStr = hours > 0 ? `${hours}h ${minutes}m` : `${minutes}m`;

  const stats = [
    {
      icon: Flame,
      value: streak.current,
      label: 'Streak',
      color: 'text-orange-500',
      bgColor: 'from-orange-50 to-amber-50 dark:from-orange-900/20 dark:to-amber-900/20',
      suffix: streak.current === 1 ? 'giorno' : 'giorni',
    },
    {
      icon: BookOpen,
      value: sessionsThisWeek,
      label: 'Questa settimana',
      color: 'text-blue-500',
      bgColor: 'from-blue-50 to-indigo-50 dark:from-blue-900/20 dark:to-indigo-900/20',
      suffix: sessionsThisWeek === 1 ? 'sessione' : 'sessioni',
    },
    {
      icon: Clock,
      value: studyTimeStr,
      label: 'Tempo totale',
      color: 'text-green-500',
      bgColor: 'from-green-50 to-emerald-50 dark:from-green-900/20 dark:to-emerald-900/20',
      suffix: '',
    },
    {
      icon: Star,
      value: questionsAsked,
      label: 'Domande',
      color: 'text-purple-500',
      bgColor: 'from-purple-50 to-violet-50 dark:from-purple-900/20 dark:to-violet-900/20',
      suffix: 'fatte',
    },
  ];

  return (
    <Card className="mb-6 overflow-hidden">
      <CardContent className="p-0">
        {/* Level and XP header */}
        <div className="p-4 bg-gradient-to-r from-blue-500 to-indigo-600 text-white">
          <div className="flex items-center justify-between mb-3">
            <div className="flex items-center gap-3">
              <div className="w-12 h-12 rounded-full bg-white/20 flex items-center justify-center">
                <Trophy className="w-6 h-6" />
              </div>
              <div>
                <p className="text-sm opacity-80">Livello</p>
                <p className="text-2xl font-bold">{level}</p>
              </div>
            </div>
            <div className="text-right">
              <p className="text-sm opacity-80">XP Totali</p>
              <p className="text-2xl font-bold">{xp.toLocaleString()}</p>
            </div>
          </div>

          {/* XP Progress bar */}
          <div className="space-y-1">
            <div className="flex justify-between text-xs opacity-80">
              <span>Livello {level}</span>
              <span>{xpInLevel.toLocaleString()} / {xpNeeded.toLocaleString()} XP</span>
              <span>Livello {level + 1}</span>
            </div>
            <div className="h-2 bg-white/20 rounded-full overflow-hidden">
              <motion.div
                className="h-full bg-white rounded-full"
                initial={{ width: 0 }}
                animate={{ width: `${progressPercent}%` }}
                transition={{ duration: 1, ease: 'easeOut' }}
              />
            </div>
          </div>
        </div>

        {/* Stats grid */}
        <div className="grid grid-cols-2 md:grid-cols-4 gap-3 p-4">
          {stats.map((stat, i) => (
            <motion.div
              key={stat.label}
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: i * 0.1 }}
              className={cn(
                'p-3 rounded-xl bg-gradient-to-br',
                stat.bgColor
              )}
            >
              <stat.icon className={cn('w-5 h-5 mb-1', stat.color)} />
              <p className={cn('text-xl font-bold', stat.color)}>
                {stat.value}
              </p>
              <p className="text-xs text-slate-600 dark:text-slate-400">
                {stat.label}
                {stat.suffix && <span className="block text-[10px] opacity-70">{stat.suffix}</span>}
              </p>
            </motion.div>
          ))}
        </div>

        {/* Streak bonus info */}
        {streak.current >= 3 && (
          <div className="px-4 pb-4">
            <div className="p-3 rounded-xl bg-gradient-to-r from-amber-100 to-orange-100 dark:from-amber-900/30 dark:to-orange-900/30 border border-amber-200 dark:border-amber-800">
              <div className="flex items-center gap-2">
                <Flame className="w-5 h-5 text-orange-500" />
                <span className="text-sm font-medium text-amber-800 dark:text-amber-300">
                  Streak bonus attivo! +{Math.min(streak.current * 10, 50)}% XP
                </span>
              </div>
            </div>
          </div>
        )}

        {/* Longest streak */}
        {streak.longest > 0 && streak.longest > streak.current && (
          <div className="px-4 pb-4 -mt-2">
            <p className="text-xs text-slate-500 flex items-center gap-1">
              <TrendingUp className="w-3 h-3" />
              Record: {streak.longest} giorni di fila
            </p>
          </div>
        )}
      </CardContent>
    </Card>
  );
}
