'use client';

import { useState, useMemo } from 'react';
import { Search, Filter } from 'lucide-react';
import { motion } from 'framer-motion';
import { MaestroCard } from './maestro-card';
import { VoiceSession } from '@/components/voice/voice-session';
import { maestri, subjectNames, getAllSubjects } from '@/data/maestri';
import { cn } from '@/lib/utils';
import type { Maestro, Subject } from '@/types';

export function MaestriGrid() {
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedSubject, setSelectedSubject] = useState<Subject | 'all'>('all');
  const [selectedMaestro, setSelectedMaestro] = useState<Maestro | null>(null);
  const [showVoiceSession, setShowVoiceSession] = useState(false);

  const subjects = getAllSubjects();

  const filteredMaestri = useMemo(() => {
    return maestri.filter((m) => {
      const matchesSearch =
        m.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
        m.specialty.toLowerCase().includes(searchQuery.toLowerCase()) ||
        subjectNames[m.subject].toLowerCase().includes(searchQuery.toLowerCase());

      const matchesSubject =
        selectedSubject === 'all' || m.subject === selectedSubject;

      return matchesSearch && matchesSubject;
    });
  }, [searchQuery, selectedSubject]);

  const handleVoiceSession = (maestro: Maestro) => {
    setSelectedMaestro(maestro);
    setShowVoiceSession(true);
  };

  const handleChat = (maestro: Maestro) => {
    // TODO: Navigate to chat with this maestro
    console.log('Chat with:', maestro.name);
  };

  const handleLearn = (maestro: Maestro) => {
    // TODO: Navigate to learning materials for this subject
    console.log('Learn from:', maestro.name);
  };

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex flex-col md:flex-row gap-4 items-start md:items-center justify-between">
        <div>
          <h1 className="text-3xl font-bold text-slate-900 dark:text-white">
            I Tuoi Maestri
          </h1>
          <p className="text-slate-600 dark:text-slate-400 mt-1">
            Scegli un maestro per iniziare a studiare
          </p>
        </div>

        {/* Search */}
        <div className="relative w-full md:w-80">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 h-4 w-4 text-slate-400" />
          <input
            type="text"
            placeholder="Cerca maestro o materia..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full pl-10 pr-4 py-2.5 rounded-xl bg-white dark:bg-slate-800 border border-slate-200 dark:border-slate-700 focus:outline-none focus:ring-2 focus:ring-blue-500 text-sm"
          />
        </div>
      </div>

      {/* Subject filters */}
      <div className="flex flex-wrap gap-2">
        <button
          onClick={() => setSelectedSubject('all')}
          className={cn(
            'px-4 py-2 rounded-full text-sm font-medium transition-all',
            selectedSubject === 'all'
              ? 'bg-blue-500 text-white'
              : 'bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 hover:bg-slate-200 dark:hover:bg-slate-700'
          )}
        >
          Tutti ({maestri.length})
        </button>
        {subjects.map((subject) => {
          const count = maestri.filter((m) => m.subject === subject).length;
          return (
            <button
              key={subject}
              onClick={() => setSelectedSubject(subject)}
              className={cn(
                'px-4 py-2 rounded-full text-sm font-medium transition-all',
                selectedSubject === subject
                  ? 'text-white'
                  : 'bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 hover:bg-slate-200 dark:hover:bg-slate-700'
              )}
              style={
                selectedSubject === subject
                  ? { backgroundColor: maestri.find((m) => m.subject === subject)?.color }
                  : {}
              }
            >
              {subjectNames[subject]} ({count})
            </button>
          );
        })}
      </div>

      {/* Grid */}
      <motion.div
        layout
        className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6"
      >
        {filteredMaestri.map((maestro, index) => (
          <motion.div
            key={maestro.id}
            layout
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: -20 }}
            transition={{ delay: index * 0.05 }}
          >
            <MaestroCard
              maestro={maestro}
              onVoiceSession={handleVoiceSession}
              onChat={handleChat}
              onLearn={handleLearn}
            />
          </motion.div>
        ))}
      </motion.div>

      {/* Empty state */}
      {filteredMaestri.length === 0 && (
        <div className="text-center py-12">
          <Filter className="h-12 w-12 mx-auto text-slate-400 mb-4" />
          <h3 className="text-lg font-medium text-slate-700 dark:text-slate-300">
            Nessun maestro trovato
          </h3>
          <p className="text-slate-500 dark:text-slate-400 mt-1">
            Prova a modificare i filtri di ricerca
          </p>
        </div>
      )}

      {/* Voice session modal */}
      {showVoiceSession && selectedMaestro && (
        <VoiceSession
          maestro={selectedMaestro}
          onClose={() => {
            setShowVoiceSession(false);
            setSelectedMaestro(null);
          }}
        />
      )}
    </div>
  );
}
