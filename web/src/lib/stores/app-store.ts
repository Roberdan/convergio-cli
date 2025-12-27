// ============================================================================
// CONVERGIO WEB - MAIN APPLICATION STORE (Zustand)
// ============================================================================

import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type {
  Maestro,
  Settings,
  Theme,
  AIProvider,
  StudentProfile,
  Progress,
  Streak,
  Achievement,
  SubjectMastery,
  Conversation,
  ChatMessage,
  ToolCall,
} from '@/types';

// === SETTINGS STORE ===

interface AppearanceSettings {
  theme: 'light' | 'dark' | 'system';
  accentColor: string;
  language: 'it' | 'en' | 'es' | 'fr' | 'de';
}

interface ExtendedStudentProfile {
  name: string;
  age: number;
  schoolYear: number;
  schoolLevel: 'elementare' | 'media' | 'superiore';
  gradeLevel: string;
  learningGoals: string[];
  fontSize: 'small' | 'medium' | 'large' | 'extra-large';
  highContrast: boolean;
  dyslexiaFont: boolean;
  voiceEnabled: boolean;
  simplifiedLanguage: boolean;
  adhdMode: boolean;
}

interface SettingsState {
  theme: Theme;
  provider: AIProvider;
  model: string;
  budgetLimit: number;
  totalSpent: number;
  studentProfile: ExtendedStudentProfile;
  appearance: AppearanceSettings;
  // Actions
  setTheme: (theme: Theme) => void;
  setProvider: (provider: AIProvider) => void;
  setModel: (model: string) => void;
  setBudgetLimit: (limit: number) => void;
  addCost: (cost: number) => void;
  updateStudentProfile: (profile: Partial<ExtendedStudentProfile>) => void;
  updateAppearance: (appearance: Partial<AppearanceSettings>) => void;
}

export const useSettingsStore = create<SettingsState>()(
  persist(
    (set) => ({
      theme: 'system',
      provider: 'openai',
      model: 'gpt-4o',
      budgetLimit: 50,
      totalSpent: 0,
      studentProfile: {
        name: '',
        age: 14,
        schoolYear: 1,
        schoolLevel: 'superiore',
        gradeLevel: '',
        learningGoals: [],
        fontSize: 'medium',
        highContrast: false,
        dyslexiaFont: false,
        voiceEnabled: true,
        simplifiedLanguage: false,
        adhdMode: false,
      },
      appearance: {
        theme: 'system',
        accentColor: 'blue',
        language: 'it',
      },
      setTheme: (theme) => set({ theme }),
      setProvider: (provider) => set({ provider }),
      setModel: (model) => set({ model }),
      setBudgetLimit: (budgetLimit) => set({ budgetLimit }),
      addCost: (cost) =>
        set((state) => ({ totalSpent: state.totalSpent + cost })),
      updateStudentProfile: (profile) =>
        set((state) => ({
          studentProfile: { ...state.studentProfile, ...profile },
        })),
      updateAppearance: (appearance) =>
        set((state) => ({
          appearance: { ...state.appearance, ...appearance },
        })),
    }),
    { name: 'convergio-settings' }
  )
);

// === PROGRESS STORE (Gamification) ===

interface StudySession {
  id: string;
  maestroId: string;
  subject: string;
  startedAt: Date;
  endedAt?: Date;
  durationMinutes?: number;
  questionsAsked: number;
  xpEarned: number;
}

interface ProgressState {
  xp: number;
  level: number;
  streak: Streak;
  masteries: SubjectMastery[];
  achievements: Achievement[];
  totalStudyMinutes: number;
  sessionsThisWeek: number;
  questionsAsked: number;
  // Session tracking
  currentSession: StudySession | null;
  sessionHistory: StudySession[];
  // Sync state
  lastSyncedAt: Date | null;
  pendingSync: boolean;
  // Actions
  addXP: (amount: number) => void;
  updateStreak: () => void;
  updateMastery: (subjectMastery: SubjectMastery) => void;
  unlockAchievement: (achievementId: string) => void;
  addStudyMinutes: (minutes: number) => void;
  incrementQuestions: () => void;
  // Session actions
  startSession: (maestroId: string, subject: string) => void;
  endSession: () => void;
  // Sync actions
  syncToServer: () => Promise<void>;
  loadFromServer: () => Promise<void>;
}

const XP_PER_LEVEL = [
  0, 100, 250, 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
];

export const useProgressStore = create<ProgressState>()(
  persist(
    (set, get) => ({
      xp: 0,
      level: 1,
      streak: { current: 0, longest: 0 },
      masteries: [],
      achievements: [],
      totalStudyMinutes: 0,
      sessionsThisWeek: 0,
      questionsAsked: 0,
      currentSession: null,
      sessionHistory: [],
      lastSyncedAt: null,
      pendingSync: false,

      addXP: (amount) =>
        set((state) => {
          const newXP = state.xp + amount;
          let newLevel = state.level;
          while (
            newLevel < XP_PER_LEVEL.length - 1 &&
            newXP >= XP_PER_LEVEL[newLevel]
          ) {
            newLevel++;
          }
          // Update current session XP
          const updatedSession = state.currentSession
            ? { ...state.currentSession, xpEarned: state.currentSession.xpEarned + amount }
            : null;
          return { xp: newXP, level: newLevel, currentSession: updatedSession, pendingSync: true };
        }),

      updateStreak: () =>
        set((state) => {
          const today = new Date().toDateString();
          const lastStudy = state.streak.lastStudyDate;
          const yesterday = new Date(Date.now() - 86400000).toDateString();

          let newCurrent = state.streak.current;
          if (!lastStudy || new Date(lastStudy).toDateString() !== today) {
            if (lastStudy && new Date(lastStudy).toDateString() === yesterday) {
              newCurrent++;
            } else if (!lastStudy || new Date(lastStudy).toDateString() !== yesterday) {
              newCurrent = 1;
            }
          }

          return {
            streak: {
              current: newCurrent,
              longest: Math.max(state.streak.longest, newCurrent),
              lastStudyDate: new Date(),
            },
            pendingSync: true,
          };
        }),

      updateMastery: (subjectMastery) =>
        set((state) => {
          const existing = state.masteries.findIndex(
            (m) => m.subject === subjectMastery.subject
          );
          if (existing >= 0) {
            const newMasteries = [...state.masteries];
            newMasteries[existing] = subjectMastery;
            return { masteries: newMasteries, pendingSync: true };
          }
          return { masteries: [...state.masteries, subjectMastery], pendingSync: true };
        }),

      unlockAchievement: (achievementId) =>
        set((state) => {
          const achievement = state.achievements.find(
            (a) => a.id === achievementId
          );
          if (achievement && !achievement.unlockedAt) {
            return {
              achievements: state.achievements.map((a) =>
                a.id === achievementId ? { ...a, unlockedAt: new Date() } : a
              ),
              pendingSync: true,
            };
          }
          return state;
        }),

      addStudyMinutes: (minutes) =>
        set((state) => ({
          totalStudyMinutes: state.totalStudyMinutes + minutes,
          pendingSync: true,
        })),

      incrementQuestions: () =>
        set((state) => {
          const updatedSession = state.currentSession
            ? { ...state.currentSession, questionsAsked: state.currentSession.questionsAsked + 1 }
            : null;
          return {
            questionsAsked: state.questionsAsked + 1,
            currentSession: updatedSession,
            pendingSync: true,
          };
        }),

      startSession: (maestroId, subject) =>
        set((state) => {
          // End any existing session first
          if (state.currentSession) {
            const endedSession = {
              ...state.currentSession,
              endedAt: new Date(),
              durationMinutes: Math.round(
                (Date.now() - new Date(state.currentSession.startedAt).getTime()) / 60000
              ),
            };
            return {
              currentSession: {
                id: crypto.randomUUID(),
                maestroId,
                subject,
                startedAt: new Date(),
                questionsAsked: 0,
                xpEarned: 0,
              },
              sessionHistory: [endedSession, ...state.sessionHistory].slice(0, 100),
              pendingSync: true,
            };
          }
          return {
            currentSession: {
              id: crypto.randomUUID(),
              maestroId,
              subject,
              startedAt: new Date(),
              questionsAsked: 0,
              xpEarned: 0,
            },
            pendingSync: true,
          };
        }),

      endSession: () =>
        set((state) => {
          if (!state.currentSession) return state;

          const durationMinutes = Math.round(
            (Date.now() - new Date(state.currentSession.startedAt).getTime()) / 60000
          );
          const endedSession = {
            ...state.currentSession,
            endedAt: new Date(),
            durationMinutes,
          };

          // Calculate week sessions
          const weekAgo = new Date(Date.now() - 7 * 24 * 60 * 60 * 1000);
          const recentSessions = [endedSession, ...state.sessionHistory].filter(
            (s) => new Date(s.startedAt) > weekAgo
          );

          return {
            currentSession: null,
            sessionHistory: [endedSession, ...state.sessionHistory].slice(0, 100),
            totalStudyMinutes: state.totalStudyMinutes + durationMinutes,
            sessionsThisWeek: recentSessions.length,
            pendingSync: true,
          };
        }),

      syncToServer: async () => {
        const state = get();
        if (!state.pendingSync) return;

        try {
          const response = await fetch('/api/progress/sync', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              xp: state.xp,
              level: state.level,
              streak: state.streak,
              masteries: state.masteries,
              achievements: state.achievements,
              totalStudyMinutes: state.totalStudyMinutes,
              sessionsThisWeek: state.sessionsThisWeek,
              questionsAsked: state.questionsAsked,
              sessionHistory: state.sessionHistory.slice(0, 20),
            }),
          });

          if (response.ok) {
            set({ lastSyncedAt: new Date(), pendingSync: false });
          }
        } catch (error) {
          console.error('Progress sync failed:', error);
        }
      },

      loadFromServer: async () => {
        try {
          const response = await fetch('/api/progress');
          if (response.ok) {
            const data = await response.json();
            set({
              xp: data.xp ?? get().xp,
              level: data.level ?? get().level,
              streak: data.streak ?? get().streak,
              masteries: data.masteries ?? get().masteries,
              achievements: data.achievements ?? get().achievements,
              totalStudyMinutes: data.totalStudyMinutes ?? get().totalStudyMinutes,
              sessionsThisWeek: data.sessionsThisWeek ?? get().sessionsThisWeek,
              questionsAsked: data.questionsAsked ?? get().questionsAsked,
              lastSyncedAt: new Date(),
              pendingSync: false,
            });
          }
        } catch (error) {
          console.error('Progress load failed:', error);
        }
      },
    }),
    { name: 'convergio-progress' }
  )
);

// === VOICE SESSION STORE ===

interface VoiceSessionState {
  isConnected: boolean;
  isListening: boolean;
  isSpeaking: boolean;
  isMuted: boolean;
  currentMaestro: Maestro | null;
  transcript: Array<{ role: 'user' | 'assistant'; content: string; timestamp: Date }>;
  toolCalls: ToolCall[];
  inputLevel: number;
  outputLevel: number;
  // Actions
  setConnected: (connected: boolean) => void;
  setListening: (listening: boolean) => void;
  setSpeaking: (speaking: boolean) => void;
  setMuted: (muted: boolean) => void;
  setCurrentMaestro: (maestro: Maestro | null) => void;
  addTranscript: (role: 'user' | 'assistant', content: string) => void;
  clearTranscript: () => void;
  addToolCall: (toolCall: ToolCall) => void;
  updateToolCall: (id: string, updates: Partial<ToolCall>) => void;
  clearToolCalls: () => void;
  setInputLevel: (level: number) => void;
  setOutputLevel: (level: number) => void;
  reset: () => void;
}

export const useVoiceSessionStore = create<VoiceSessionState>((set) => ({
  isConnected: false,
  isListening: false,
  isSpeaking: false,
  isMuted: false,
  currentMaestro: null,
  transcript: [],
  toolCalls: [],
  inputLevel: 0,
  outputLevel: 0,

  setConnected: (isConnected) => set({ isConnected }),
  setListening: (isListening) => set({ isListening }),
  setSpeaking: (isSpeaking) => set({ isSpeaking }),
  setMuted: (isMuted) => set({ isMuted }),
  setCurrentMaestro: (currentMaestro) => set({ currentMaestro }),
  addTranscript: (role, content) =>
    set((state) => ({
      transcript: [...state.transcript, { role, content, timestamp: new Date() }],
    })),
  clearTranscript: () => set({ transcript: [] }),
  addToolCall: (toolCall) =>
    set((state) => ({
      toolCalls: [...state.toolCalls, toolCall],
    })),
  updateToolCall: (id, updates) =>
    set((state) => ({
      toolCalls: state.toolCalls.map((tc) =>
        tc.id === id ? { ...tc, ...updates } : tc
      ),
    })),
  clearToolCalls: () => set({ toolCalls: [] }),
  setInputLevel: (inputLevel) => set({ inputLevel }),
  setOutputLevel: (outputLevel) => set({ outputLevel }),
  reset: () =>
    set({
      isConnected: false,
      isListening: false,
      isSpeaking: false,
      isMuted: false,
      currentMaestro: null,
      transcript: [],
      toolCalls: [],
      inputLevel: 0,
      outputLevel: 0,
    }),
}));

// === CONVERSATION STORE ===

interface ConversationState {
  conversations: Conversation[];
  currentConversationId: string | null;
  // Actions
  createConversation: (maestroId?: string) => string;
  addMessage: (conversationId: string, message: Omit<ChatMessage, 'id' | 'timestamp'>) => void;
  setCurrentConversation: (id: string | null) => void;
  deleteConversation: (id: string) => void;
  clearConversations: () => void;
}

export const useConversationStore = create<ConversationState>()(
  persist(
    (set, get) => ({
      conversations: [],
      currentConversationId: null,

      createConversation: (maestroId) => {
        const id = crypto.randomUUID();
        const conversation: Conversation = {
          id,
          title: 'New Conversation',
          messages: [],
          maestroId,
          createdAt: new Date(),
          updatedAt: new Date(),
        };
        set((state) => ({
          conversations: [conversation, ...state.conversations],
          currentConversationId: id,
        }));
        return id;
      },

      addMessage: (conversationId, message) =>
        set((state) => ({
          conversations: state.conversations.map((conv) =>
            conv.id === conversationId
              ? {
                  ...conv,
                  messages: [
                    ...conv.messages,
                    { ...message, id: crypto.randomUUID(), timestamp: new Date() },
                  ],
                  updatedAt: new Date(),
                  title:
                    conv.messages.length === 0 && message.role === 'user'
                      ? message.content.slice(0, 50)
                      : conv.title,
                }
              : conv
          ),
        })),

      setCurrentConversation: (id) => set({ currentConversationId: id }),

      deleteConversation: (id) =>
        set((state) => ({
          conversations: state.conversations.filter((c) => c.id !== id),
          currentConversationId:
            state.currentConversationId === id
              ? null
              : state.currentConversationId,
        })),

      clearConversations: () =>
        set({ conversations: [], currentConversationId: null }),
    }),
    { name: 'convergio-conversations' }
  )
);

// === UI STORE ===

interface UIState {
  sidebarOpen: boolean;
  settingsOpen: boolean;
  currentView: 'maestri' | 'chat' | 'voice' | 'quiz' | 'flashcards' | 'homework' | 'progress';
  // Actions
  toggleSidebar: () => void;
  setSidebarOpen: (open: boolean) => void;
  toggleSettings: () => void;
  setCurrentView: (view: UIState['currentView']) => void;
}

export const useUIStore = create<UIState>((set) => ({
  sidebarOpen: true,
  settingsOpen: false,
  currentView: 'maestri',
  toggleSidebar: () => set((state) => ({ sidebarOpen: !state.sidebarOpen })),
  setSidebarOpen: (sidebarOpen) => set({ sidebarOpen }),
  toggleSettings: () => set((state) => ({ settingsOpen: !state.settingsOpen })),
  setCurrentView: (currentView) => set({ currentView }),
}));
