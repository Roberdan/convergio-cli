'use client';

import { useState, useMemo } from 'react';
import { Search, Filter, Mic, MessageSquare, X } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import Image from 'next/image';
import { MaestroCard } from './maestro-card';
import { VoiceSession } from '@/components/voice/voice-session';
import { ChatSession } from '@/components/chat/chat-session';
import { maestri, subjectNames, subjectIcons, subjectColors, getAllSubjects } from '@/data/maestri';
import { cn } from '@/lib/utils';
import type { Maestro, Subject } from '@/types';

type SessionMode = 'voice' | 'chat' | null;

export function MaestriGrid() {
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedSubject, setSelectedSubject] = useState<Subject | 'all'>('all');
  const [selectedMaestro, setSelectedMaestro] = useState<Maestro | null>(null);
  const [showModeSelector, setShowModeSelector] = useState(false);
  const [sessionMode, setSessionMode] = useState<SessionMode>(null);

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

  const handleSelect = (maestro: Maestro) => {
    setSelectedMaestro(maestro);
    setShowModeSelector(true);
  };

  const handleModeSelect = (mode: 'voice' | 'chat') => {
    setShowModeSelector(false);
    setSessionMode(mode);
  };

  const handleCloseSession = () => {
    setSessionMode(null);
    setSelectedMaestro(null);
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
            'inline-flex items-center gap-1.5 px-4 py-2 rounded-full text-sm font-medium transition-all',
            selectedSubject === 'all'
              ? 'bg-blue-500 text-white'
              : 'bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 hover:bg-slate-200 dark:hover:bg-slate-700'
          )}
        >
          ✨ Tutti ({maestri.length})
        </button>
        {subjects.map((subject) => {
          const count = maestri.filter((m) => m.subject === subject).length;
          const isSelected = selectedSubject === subject;
          return (
            <button
              key={subject}
              onClick={() => setSelectedSubject(subject)}
              className={cn(
                'inline-flex items-center gap-1.5 px-4 py-2 rounded-full text-sm font-medium transition-all',
                isSelected
                  ? 'text-white'
                  : 'bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 hover:bg-slate-200 dark:hover:bg-slate-700'
              )}
              style={
                isSelected
                  ? { backgroundColor: subjectColors[subject] }
                  : {}
              }
            >
              <span>{subjectIcons[subject]}</span>
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
              onSelect={handleSelect}
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

      {/* Mode selector modal */}
      <AnimatePresence>
        {showModeSelector && selectedMaestro && (
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 p-4"
            onClick={() => setShowModeSelector(false)}
          >
            <motion.div
              initial={{ scale: 0.9, opacity: 0 }}
              animate={{ scale: 1, opacity: 1 }}
              exit={{ scale: 0.9, opacity: 0 }}
              onClick={(e) => e.stopPropagation()}
              className="bg-white dark:bg-slate-900 rounded-2xl p-6 max-w-md w-full shadow-2xl"
            >
              {/* Maestro header */}
              <div className="flex items-center gap-4 mb-6">
                <div
                  className="w-16 h-16 rounded-full overflow-hidden"
                  style={{ boxShadow: `0 0 0 3px ${selectedMaestro.color}` }}
                >
                  <Image
                    src={selectedMaestro.avatar}
                    alt={selectedMaestro.name}
                    width={64}
                    height={64}
                    className="w-full h-full object-cover"
                  />
                </div>
                <div>
                  <h3 className="text-xl font-bold text-slate-900 dark:text-white">
                    {selectedMaestro.name}
                  </h3>
                  <p className="text-slate-600 dark:text-slate-400">
                    {selectedMaestro.specialty}
                  </p>
                </div>
                <button
                  onClick={() => setShowModeSelector(false)}
                  className="ml-auto p-2 rounded-full hover:bg-slate-100 dark:hover:bg-slate-800"
                >
                  <X className="w-5 h-5 text-slate-500" />
                </button>
              </div>

              <p className="text-slate-600 dark:text-slate-400 mb-6">
                Come vuoi interagire con {selectedMaestro.name}?
              </p>

              {/* Mode buttons */}
              <div className="grid grid-cols-2 gap-4">
                <button
                  onClick={() => handleModeSelect('voice')}
                  className="flex flex-col items-center gap-3 p-6 rounded-xl border-2 border-slate-200 dark:border-slate-700 hover:border-blue-500 hover:bg-blue-50 dark:hover:bg-blue-900/20 transition-all group"
                >
                  <div className="w-14 h-14 rounded-full bg-blue-100 dark:bg-blue-900/30 flex items-center justify-center group-hover:bg-blue-200 dark:group-hover:bg-blue-900/50 transition-colors">
                    <Mic className="w-7 h-7 text-blue-600" />
                  </div>
                  <span className="font-semibold text-slate-900 dark:text-white">
                    Voce
                  </span>
                  <span className="text-xs text-slate-500 text-center">
                    Parla direttamente
                  </span>
                </button>

                <button
                  onClick={() => handleModeSelect('chat')}
                  className="flex flex-col items-center gap-3 p-6 rounded-xl border-2 border-slate-200 dark:border-slate-700 hover:border-green-500 hover:bg-green-50 dark:hover:bg-green-900/20 transition-all group"
                >
                  <div className="w-14 h-14 rounded-full bg-green-100 dark:bg-green-900/30 flex items-center justify-center group-hover:bg-green-200 dark:group-hover:bg-green-900/50 transition-colors">
                    <MessageSquare className="w-7 h-7 text-green-600" />
                  </div>
                  <span className="font-semibold text-slate-900 dark:text-white">
                    Chat
                  </span>
                  <span className="text-xs text-slate-500 text-center">
                    Scrivi messaggi
                  </span>
                </button>
              </div>
            </motion.div>
          </motion.div>
        )}
      </AnimatePresence>

      {/* Voice session */}
      {sessionMode === 'voice' && selectedMaestro && (
        <VoiceSession
          maestro={selectedMaestro}
          onClose={handleCloseSession}
        />
      )}

      {/* Chat session */}
      {sessionMode === 'chat' && selectedMaestro && (
        <ChatSession
          maestro={selectedMaestro}
          onClose={handleCloseSession}
          onSwitchToVoice={() => {
            setSessionMode('voice');
          }}
        />
      )}
    </div>
  );
}
