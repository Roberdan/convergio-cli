'use client';

import { motion } from 'framer-motion';
import { Mic, BookOpen, MessageCircle } from 'lucide-react';
import { Card, CardContent } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { cn } from '@/lib/utils';
import { subjectNames } from '@/data/maestri';
import type { Maestro } from '@/types';

interface MaestroCardProps {
  maestro: Maestro;
  onVoiceSession: (maestro: Maestro) => void;
  onChat: (maestro: Maestro) => void;
  onLearn: (maestro: Maestro) => void;
}

export function MaestroCard({
  maestro,
  onVoiceSession,
  onChat,
  onLearn,
}: MaestroCardProps) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      whileHover={{ y: -4 }}
      transition={{ type: 'spring', stiffness: 400, damping: 25 }}
    >
      <Card className="overflow-hidden hover:shadow-xl transition-shadow duration-300 group">
        {/* Header with color */}
        <div
          className="h-24 relative overflow-hidden"
          style={{ backgroundColor: maestro.color }}
        >
          {/* Decorative pattern */}
          <div className="absolute inset-0 opacity-20">
            <div className="absolute top-0 right-0 w-32 h-32 rounded-full bg-white/30 -translate-y-1/2 translate-x-1/2" />
            <div className="absolute bottom-0 left-0 w-24 h-24 rounded-full bg-white/20 translate-y-1/2 -translate-x-1/2" />
          </div>

          {/* Avatar */}
          <div className="absolute -bottom-10 left-6">
            <div
              className="w-20 h-20 rounded-2xl flex items-center justify-center text-3xl font-bold text-white shadow-lg ring-4 ring-white dark:ring-slate-900"
              style={{ backgroundColor: maestro.color }}
            >
              {maestro.name[0]}
            </div>
          </div>
        </div>

        <CardContent className="pt-14 pb-6 px-6">
          {/* Info */}
          <div className="mb-4">
            <h3 className="text-xl font-bold text-slate-900 dark:text-white">
              {maestro.name}
            </h3>
            <p className="text-sm font-medium" style={{ color: maestro.color }}>
              {subjectNames[maestro.subject]}
            </p>
            <p className="text-sm text-slate-500 dark:text-slate-400 mt-1">
              {maestro.specialty}
            </p>
          </div>

          {/* Teaching style */}
          <p className="text-xs text-slate-600 dark:text-slate-400 mb-4 line-clamp-2">
            {maestro.teachingStyle}
          </p>

          {/* Actions */}
          <div className="flex gap-2">
            <Button
              onClick={() => onVoiceSession(maestro)}
              className="flex-1"
              style={{ backgroundColor: maestro.color }}
            >
              <Mic className="h-4 w-4 mr-1" />
              Voce
            </Button>
            <Button
              variant="outline"
              size="icon"
              onClick={() => onChat(maestro)}
              className="hover:border-slate-400"
            >
              <MessageCircle className="h-4 w-4" />
            </Button>
            <Button
              variant="outline"
              size="icon"
              onClick={() => onLearn(maestro)}
              className="hover:border-slate-400"
            >
              <BookOpen className="h-4 w-4" />
            </Button>
          </div>
        </CardContent>
      </Card>
    </motion.div>
  );
}
