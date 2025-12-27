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
} from '@/types';

// === SETTINGS STORE ===

interface SettingsState {
  theme: Theme;
  provider: AIProvider;
  model: string;
  budgetLimit: number;
  totalSpent: number;
  studentProfile: StudentProfile;
  // Actions
  setTheme: (theme: Theme) => void;
  setProvider: (provider: AIProvider) => void;
  setModel: (model: string) => void;
  setBudgetLimit: (limit: number) => void;
  addCost: (cost: number) => void;
  updateStudentProfile: (profile: Partial<StudentProfile>) => void;
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
        fontSize: 'medium',
        highContrast: false,
        dyslexiaFont: false,
        voiceEnabled: true,
        simplifiedLanguage: false,
        adhdMode: false,
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
    }),
    { name: 'convergio-settings' }
  )
);

// === PROGRESS STORE (Gamification) ===

interface ProgressState {
  xp: number;
  level: number;
  streak: Streak;
  masteries: SubjectMastery[];
  achievements: Achievement[];
  totalStudyMinutes: number;
  sessionsThisWeek: number;
  questionsAsked: number;
  // Actions
  addXP: (amount: number) => void;
  updateStreak: () => void;
  updateMastery: (subjectMastery: SubjectMastery) => void;
  unlockAchievement: (achievementId: string) => void;
  addStudyMinutes: (minutes: number) => void;
  incrementQuestions: () => void;
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
          return { xp: newXP, level: newLevel };
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
            return { masteries: newMasteries };
          }
          return { masteries: [...state.masteries, subjectMastery] };
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
            };
          }
          return state;
        }),

      addStudyMinutes: (minutes) =>
        set((state) => ({
          totalStudyMinutes: state.totalStudyMinutes + minutes,
        })),

      incrementQuestions: () =>
        set((state) => ({ questionsAsked: state.questionsAsked + 1 })),
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
