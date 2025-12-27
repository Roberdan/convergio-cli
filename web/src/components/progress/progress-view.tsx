'use client';

import { useState, useMemo } from 'react';
import { motion } from 'framer-motion';
import {
  Trophy,
  Flame,
  TrendingUp,
  Clock,
  Star,
  Target,
  BookOpen,
  Brain,
  Award,
  Calendar,
  ChevronRight,
  Zap,
} from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { useProgressStore } from '@/lib/stores/app-store';
import { cn } from '@/lib/utils';
import { subjectNames, subjectColors, subjectIcons } from '@/data/maestri';
import type { Subject } from '@/types';

type ProgressTab = 'overview' | 'achievements' | 'mastery' | 'history';

const ACHIEVEMENTS = [
  { id: 'first_session', name: 'Prima Lezione', desc: 'Completa la tua prima sessione', icon: '🎓', xp: 50 },
  { id: 'streak_3', name: 'Costanza', desc: 'Mantieni una streak di 3 giorni', icon: '🔥', xp: 100 },
  { id: 'streak_7', name: 'Determinazione', desc: 'Mantieni una streak di 7 giorni', icon: '💪', xp: 250 },
  { id: 'streak_30', name: 'Inarrestabile', desc: 'Mantieni una streak di 30 giorni', icon: '🚀', xp: 1000 },
  { id: 'quiz_10', name: 'Quiz Master', desc: 'Completa 10 quiz', icon: '🧠', xp: 150 },
  { id: 'flashcards_100', name: 'Memoria di Ferro', desc: 'Rivedi 100 flashcards', icon: '📚', xp: 200 },
  { id: 'math_master', name: 'Genio Matematico', desc: 'Raggiungi livello esperto in matematica', icon: '🔢', xp: 500 },
  { id: 'polyglot', name: 'Poliglotta', desc: 'Studia 3 lingue diverse', icon: '🌍', xp: 300 },
  { id: 'night_owl', name: 'Gufo Notturno', desc: 'Studia dopo le 22:00', icon: '🦉', xp: 50 },
  { id: 'early_bird', name: 'Mattiniero', desc: 'Studia prima delle 7:00', icon: '🐦', xp: 50 },
  { id: 'study_marathon', name: 'Maratona', desc: 'Studia per 2 ore consecutive', icon: '⏱️', xp: 200 },
  { id: 'perfectionist', name: 'Perfezionista', desc: 'Ottieni 100% in un quiz', icon: '💯', xp: 100 },
];

export function ProgressView() {
  const [activeTab, setActiveTab] = useState<ProgressTab>('overview');
  const { xp, level, streak, totalStudyMinutes, subjectMastery, achievements } = useProgressStore();

  const tabs: Array<{ id: ProgressTab; label: string; icon: React.ReactNode }> = [
    { id: 'overview', label: 'Panoramica', icon: <TrendingUp className="w-4 h-4" /> },
    { id: 'achievements', label: 'Traguardi', icon: <Trophy className="w-4 h-4" /> },
    { id: 'mastery', label: 'Padronanza', icon: <Star className="w-4 h-4" /> },
    { id: 'history', label: 'Cronologia', icon: <Calendar className="w-4 h-4" /> },
  ];

  const xpToNextLevel = 1000;
  const currentLevelXP = xp % xpToNextLevel;
  const levelProgress = (currentLevelXP / xpToNextLevel) * 100;

  const unlockedAchievements = achievements || [];
  const achievementProgress = (unlockedAchievements.length / ACHIEVEMENTS.length) * 100;

  return (
    <div className="space-y-6">
      {/* Header */}
      <div>
        <h1 className="text-3xl font-bold text-slate-900 dark:text-white">
          I Tuoi Progressi
        </h1>
        <p className="text-slate-600 dark:text-slate-400 mt-1">
          Monitora il tuo percorso di apprendimento
        </p>
      </div>

      {/* Quick Stats */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <StatCard
          icon={<Zap className="w-6 h-6 text-amber-500" />}
          label="Livello"
          value={level.toString()}
          subtext={`${currentLevelXP} / ${xpToNextLevel} XP`}
          color="amber"
        />
        <StatCard
          icon={<Flame className="w-6 h-6 text-orange-500" />}
          label="Streak"
          value={`${streak.current} giorni`}
          subtext={`Record: ${streak.longest}`}
          color="orange"
        />
        <StatCard
          icon={<Clock className="w-6 h-6 text-blue-500" />}
          label="Tempo Studio"
          value={formatMinutes(totalStudyMinutes)}
          subtext="Totale"
          color="blue"
        />
        <StatCard
          icon={<Trophy className="w-6 h-6 text-purple-500" />}
          label="Traguardi"
          value={`${unlockedAchievements.length}/${ACHIEVEMENTS.length}`}
          subtext={`${achievementProgress.toFixed(0)}%`}
          color="purple"
        />
      </div>

      {/* Tabs */}
      <div className="flex gap-2 border-b border-slate-200 dark:border-slate-700 pb-4">
        {tabs.map(tab => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={cn(
              'flex items-center gap-2 px-4 py-2 rounded-lg text-sm font-medium transition-all',
              activeTab === tab.id
                ? 'bg-blue-500 text-white'
                : 'text-slate-600 dark:text-slate-400 hover:bg-slate-100 dark:hover:bg-slate-800'
            )}
          >
            {tab.icon}
            {tab.label}
          </button>
        ))}
      </div>

      {/* Tab Content */}
      <motion.div
        key={activeTab}
        initial={{ opacity: 0, y: 10 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ duration: 0.2 }}
      >
        {activeTab === 'overview' && (
          <OverviewTab
            xp={xp}
            level={level}
            levelProgress={levelProgress}
            streak={streak}
            subjectMastery={subjectMastery}
          />
        )}

        {activeTab === 'achievements' && (
          <AchievementsTab
            unlocked={unlockedAchievements}
            allAchievements={ACHIEVEMENTS}
          />
        )}

        {activeTab === 'mastery' && (
          <MasteryTab subjectMastery={subjectMastery} />
        )}

        {activeTab === 'history' && <HistoryTab />}
      </motion.div>
    </div>
  );
}

// Stat Card Component
interface StatCardProps {
  icon: React.ReactNode;
  label: string;
  value: string;
  subtext: string;
  color: 'amber' | 'orange' | 'blue' | 'purple' | 'green';
}

function StatCard({ icon, label, value, subtext, color }: StatCardProps) {
  const colorClasses = {
    amber: 'from-amber-50 to-yellow-50 dark:from-amber-900/20 dark:to-yellow-900/20',
    orange: 'from-orange-50 to-red-50 dark:from-orange-900/20 dark:to-red-900/20',
    blue: 'from-blue-50 to-indigo-50 dark:from-blue-900/20 dark:to-indigo-900/20',
    purple: 'from-purple-50 to-pink-50 dark:from-purple-900/20 dark:to-pink-900/20',
    green: 'from-green-50 to-emerald-50 dark:from-green-900/20 dark:to-emerald-900/20',
  };

  return (
    <Card className={cn('bg-gradient-to-br', colorClasses[color])}>
      <CardContent className="p-4">
        <div className="flex items-center gap-3">
          {icon}
          <div>
            <p className="text-xs text-slate-500 dark:text-slate-400">{label}</p>
            <p className="text-xl font-bold text-slate-900 dark:text-white">{value}</p>
            <p className="text-xs text-slate-500 dark:text-slate-400">{subtext}</p>
          </div>
        </div>
      </CardContent>
    </Card>
  );
}

// Overview Tab
interface OverviewTabProps {
  xp: number;
  level: number;
  levelProgress: number;
  streak: { current: number; longest: number; lastStudyDate: string | null };
  subjectMastery: Record<string, any>;
}

function OverviewTab({ xp, level, levelProgress, streak, subjectMastery }: OverviewTabProps) {
  // Simulated weekly activity data
  const weeklyData = [
    { day: 'Lun', minutes: 45 },
    { day: 'Mar', minutes: 30 },
    { day: 'Mer', minutes: 60 },
    { day: 'Gio', minutes: 15 },
    { day: 'Ven', minutes: 90 },
    { day: 'Sab', minutes: 0 },
    { day: 'Dom', minutes: 25 },
  ];

  const maxMinutes = Math.max(...weeklyData.map(d => d.minutes), 1);

  return (
    <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
      {/* Level Progress */}
      <Card>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <Zap className="w-5 h-5 text-amber-500" />
            Progressione Livello
          </CardTitle>
        </CardHeader>
        <CardContent>
          <div className="flex items-center gap-4 mb-4">
            <div className="w-16 h-16 rounded-full bg-gradient-to-br from-amber-400 to-orange-500 flex items-center justify-center text-white text-2xl font-bold">
              {level}
            </div>
            <div className="flex-1">
              <p className="text-sm text-slate-500 mb-1">
                {xp.toLocaleString()} XP totali
              </p>
              <div className="h-3 bg-slate-200 dark:bg-slate-700 rounded-full overflow-hidden">
                <motion.div
                  className="h-full bg-gradient-to-r from-amber-400 to-orange-500"
                  initial={{ width: 0 }}
                  animate={{ width: `${levelProgress}%` }}
                  transition={{ duration: 0.5 }}
                />
              </div>
              <p className="text-xs text-slate-400 mt-1">
                {(1000 - (xp % 1000)).toLocaleString()} XP per il livello {level + 1}
              </p>
            </div>
          </div>
        </CardContent>
      </Card>

      {/* Weekly Activity */}
      <Card>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <Calendar className="w-5 h-5 text-blue-500" />
            Attivita Settimanale
          </CardTitle>
        </CardHeader>
        <CardContent>
          <div className="flex items-end justify-between h-32 gap-2">
            {weeklyData.map((day, i) => (
              <div key={i} className="flex-1 flex flex-col items-center gap-1">
                <motion.div
                  className="w-full bg-blue-500 rounded-t"
                  initial={{ height: 0 }}
                  animate={{ height: `${(day.minutes / maxMinutes) * 100}%` }}
                  transition={{ duration: 0.5, delay: i * 0.05 }}
                  style={{ minHeight: day.minutes > 0 ? '8px' : '0px' }}
                />
                <span className="text-xs text-slate-500">{day.day}</span>
              </div>
            ))}
          </div>
        </CardContent>
      </Card>

      {/* Streak Calendar */}
      <Card>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <Flame className="w-5 h-5 text-orange-500" />
            Calendario Streak
          </CardTitle>
        </CardHeader>
        <CardContent>
          <div className="grid grid-cols-7 gap-2">
            {Array.from({ length: 28 }).map((_, i) => {
              const isActive = i < streak.current || Math.random() > 0.3;
              return (
                <div
                  key={i}
                  className={cn(
                    'aspect-square rounded-sm',
                    isActive
                      ? 'bg-orange-400'
                      : 'bg-slate-200 dark:bg-slate-700'
                  )}
                />
              );
            })}
          </div>
          <div className="flex items-center justify-between mt-4 text-sm">
            <span className="text-slate-500">4 settimane fa</span>
            <span className="text-slate-500">Oggi</span>
          </div>
        </CardContent>
      </Card>

      {/* Top Subjects */}
      <Card>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <BookOpen className="w-5 h-5 text-green-500" />
            Materie Principali
          </CardTitle>
        </CardHeader>
        <CardContent className="space-y-3">
          {Object.entries(subjectMastery || {}).slice(0, 4).map(([subject, data]: [string, any]) => (
            <div key={subject} className="flex items-center gap-3">
              <span className="text-2xl">{subjectIcons[subject as Subject]}</span>
              <div className="flex-1">
                <div className="flex items-center justify-between mb-1">
                  <span className="font-medium text-sm">{subjectNames[subject as Subject]}</span>
                  <span className="text-xs text-slate-500">{data?.tier || 'beginner'}</span>
                </div>
                <div className="h-2 bg-slate-200 dark:bg-slate-700 rounded-full overflow-hidden">
                  <div
                    className="h-full rounded-full"
                    style={{
                      width: `${data?.progress || 0}%`,
                      backgroundColor: subjectColors[subject as Subject],
                    }}
                  />
                </div>
              </div>
            </div>
          ))}
        </CardContent>
      </Card>
    </div>
  );
}

// Achievements Tab
interface AchievementsTabProps {
  unlocked: string[];
  allAchievements: typeof ACHIEVEMENTS;
}

function AchievementsTab({ unlocked, allAchievements }: AchievementsTabProps) {
  return (
    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
      {allAchievements.map(achievement => {
        const isUnlocked = unlocked.includes(achievement.id);
        return (
          <Card
            key={achievement.id}
            className={cn(
              'transition-all',
              isUnlocked
                ? 'bg-gradient-to-br from-amber-50 to-yellow-50 dark:from-amber-900/20 dark:to-yellow-900/20 border-amber-200 dark:border-amber-800'
                : 'opacity-60'
            )}
          >
            <CardContent className="p-4">
              <div className="flex items-start gap-3">
                <div
                  className={cn(
                    'text-4xl',
                    !isUnlocked && 'grayscale'
                  )}
                >
                  {achievement.icon}
                </div>
                <div className="flex-1">
                  <h4 className="font-bold text-slate-900 dark:text-white">
                    {achievement.name}
                  </h4>
                  <p className="text-sm text-slate-500 dark:text-slate-400">
                    {achievement.desc}
                  </p>
                  <div className="flex items-center gap-1 mt-2">
                    <Zap className="w-4 h-4 text-amber-500" />
                    <span className="text-sm font-medium text-amber-600">
                      +{achievement.xp} XP
                    </span>
                  </div>
                </div>
                {isUnlocked && (
                  <Award className="w-6 h-6 text-amber-500" />
                )}
              </div>
            </CardContent>
          </Card>
        );
      })}
    </div>
  );
}

// Mastery Tab
interface MasteryTabProps {
  subjectMastery: Record<string, any>;
}

function MasteryTab({ subjectMastery }: MasteryTabProps) {
  const tiers = ['beginner', 'intermediate', 'advanced', 'expert', 'master'];
  const tierLabels: Record<string, string> = {
    beginner: 'Principiante',
    intermediate: 'Intermedio',
    advanced: 'Avanzato',
    expert: 'Esperto',
    master: 'Maestro',
  };

  const subjects: Subject[] = ['math', 'science', 'history', 'geography', 'italian', 'english', 'art', 'music'];

  return (
    <div className="space-y-6">
      {subjects.map(subject => {
        const data = subjectMastery?.[subject] || { tier: 'beginner', progress: 0, sessionsCompleted: 0 };
        const tierIndex = tiers.indexOf(data.tier);

        return (
          <Card key={subject}>
            <CardContent className="p-4">
              <div className="flex items-center gap-4">
                <div
                  className="w-14 h-14 rounded-xl flex items-center justify-center text-3xl"
                  style={{ backgroundColor: `${subjectColors[subject]}20` }}
                >
                  {subjectIcons[subject]}
                </div>

                <div className="flex-1">
                  <div className="flex items-center justify-between mb-2">
                    <h4 className="font-bold text-slate-900 dark:text-white">
                      {subjectNames[subject]}
                    </h4>
                    <span
                      className="px-3 py-1 rounded-full text-sm font-medium"
                      style={{
                        backgroundColor: `${subjectColors[subject]}20`,
                        color: subjectColors[subject],
                      }}
                    >
                      {tierLabels[data.tier]}
                    </span>
                  </div>

                  {/* Tier Progress */}
                  <div className="flex gap-1 mb-2">
                    {tiers.map((tier, i) => (
                      <div
                        key={tier}
                        className={cn(
                          'flex-1 h-2 rounded-full',
                          i <= tierIndex
                            ? 'bg-gradient-to-r'
                            : 'bg-slate-200 dark:bg-slate-700'
                        )}
                        style={i <= tierIndex ? {
                          backgroundImage: `linear-gradient(to right, ${subjectColors[subject]}, ${subjectColors[subject]})`
                        } : {}}
                      />
                    ))}
                  </div>

                  <div className="flex items-center justify-between text-sm text-slate-500">
                    <span>{data.sessionsCompleted || 0} sessioni completate</span>
                    <span>{data.progress || 0}% al prossimo livello</span>
                  </div>
                </div>

                <ChevronRight className="w-5 h-5 text-slate-400" />
              </div>
            </CardContent>
          </Card>
        );
      })}
    </div>
  );
}

// History Tab
function HistoryTab() {
  // Simulated study history
  const history = [
    { date: 'Oggi', sessions: [{ subject: 'math', duration: 30, maestro: 'Professoressa Algebra' }] },
    { date: 'Ieri', sessions: [{ subject: 'italian', duration: 45, maestro: 'Maestro Dante' }, { subject: 'english', duration: 20, maestro: 'Mr. Shakespeare' }] },
    { date: '2 giorni fa', sessions: [{ subject: 'science', duration: 60, maestro: 'Dr. Einstein' }] },
  ];

  return (
    <div className="space-y-6">
      {history.map((day, i) => (
        <div key={i}>
          <h3 className="text-sm font-medium text-slate-500 mb-3">{day.date}</h3>
          <div className="space-y-3">
            {day.sessions.map((session, j) => (
              <Card key={j}>
                <CardContent className="p-4">
                  <div className="flex items-center gap-4">
                    <div
                      className="w-12 h-12 rounded-xl flex items-center justify-center text-2xl"
                      style={{ backgroundColor: `${subjectColors[session.subject as Subject]}20` }}
                    >
                      {subjectIcons[session.subject as Subject]}
                    </div>
                    <div className="flex-1">
                      <h4 className="font-medium text-slate-900 dark:text-white">
                        {subjectNames[session.subject as Subject]}
                      </h4>
                      <p className="text-sm text-slate-500">
                        con {session.maestro}
                      </p>
                    </div>
                    <div className="text-right">
                      <p className="font-medium text-slate-900 dark:text-white">
                        {session.duration} min
                      </p>
                      <p className="text-sm text-slate-500">
                        +{session.duration * 2} XP
                      </p>
                    </div>
                  </div>
                </CardContent>
              </Card>
            ))}
          </div>
        </div>
      ))}
    </div>
  );
}

// Utility function
function formatMinutes(minutes: number): string {
  if (minutes < 60) return `${minutes}m`;
  const hours = Math.floor(minutes / 60);
  const mins = minutes % 60;
  return mins > 0 ? `${hours}h ${mins}m` : `${hours}h`;
}
