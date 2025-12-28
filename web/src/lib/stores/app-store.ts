// ============================================================================
// CONVERGIO WEB - MAIN APPLICATION STORE (Zustand)
// With database sync via API routes
// ============================================================================

import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type {
  Maestro,
  Theme,
  AIProvider,
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
  // Sync state
  lastSyncedAt: Date | null;
  pendingSync: boolean;
  // Actions
  setTheme: (theme: Theme) => void;
  setProvider: (provider: AIProvider) => void;
  setModel: (model: string) => void;
  setBudgetLimit: (limit: number) => void;
  addCost: (cost: number) => void;
  updateStudentProfile: (profile: Partial<ExtendedStudentProfile>) => void;
  updateAppearance: (appearance: Partial<AppearanceSettings>) => void;
  // Sync actions
  syncToServer: () => Promise<void>;
  loadFromServer: () => Promise<void>;
}

export const useSettingsStore = create<SettingsState>()(
  persist(
    (set, get) => ({
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
      lastSyncedAt: null,
      pendingSync: false,

      setTheme: (theme) => set({ theme, pendingSync: true }),
      setProvider: (provider) => set({ provider, pendingSync: true }),
      setModel: (model) => set({ model, pendingSync: true }),
      setBudgetLimit: (budgetLimit) => set({ budgetLimit, pendingSync: true }),
      addCost: (cost) =>
        set((state) => ({ totalSpent: state.totalSpent + cost, pendingSync: true })),
      updateStudentProfile: (profile) =>
        set((state) => ({
          studentProfile: { ...state.studentProfile, ...profile },
          pendingSync: true,
        })),
      updateAppearance: (appearance) =>
        set((state) => ({
          appearance: { ...state.appearance, ...appearance },
          pendingSync: true,
        })),

      syncToServer: async () => {
        const state = get();
        if (!state.pendingSync) return;

        try {
          // Sync settings
          await fetch('/api/user/settings', {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              theme: state.theme,
              provider: state.provider,
              model: state.model,
              budgetLimit: state.budgetLimit,
              language: state.appearance.language,
              accentColor: state.appearance.accentColor,
            }),
          });

          // Sync profile
          await fetch('/api/user/profile', {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              name: state.studentProfile.name,
              age: state.studentProfile.age,
              schoolYear: state.studentProfile.schoolYear,
              schoolLevel: state.studentProfile.schoolLevel,
              gradeLevel: state.studentProfile.gradeLevel,
              learningGoals: state.studentProfile.learningGoals,
              accessibility: {
                fontSize: state.studentProfile.fontSize,
                highContrast: state.studentProfile.highContrast,
                dyslexiaFont: state.studentProfile.dyslexiaFont,
                voiceEnabled: state.studentProfile.voiceEnabled,
                simplifiedLanguage: state.studentProfile.simplifiedLanguage,
                adhdMode: state.studentProfile.adhdMode,
              },
            }),
          });

          set({ lastSyncedAt: new Date(), pendingSync: false });
        } catch (error) {
          console.error('Settings sync failed:', error);
        }
      },

      loadFromServer: async () => {
        try {
          const [settingsRes, profileRes] = await Promise.all([
            fetch('/api/user/settings'),
            fetch('/api/user/profile'),
          ]);

          if (settingsRes.ok) {
            const settings = await settingsRes.json();
            set((state) => ({
              theme: settings.theme ?? state.theme,
              provider: settings.provider ?? state.provider,
              model: settings.model ?? state.model,
              budgetLimit: settings.budgetLimit ?? state.budgetLimit,
              appearance: {
                ...state.appearance,
                language: settings.language ?? state.appearance.language,
                accentColor: settings.accentColor ?? state.appearance.accentColor,
              },
            }));
          }

          if (profileRes.ok) {
            const profile = await profileRes.json();
            if (profile) {
              const accessibility = profile.accessibility || {};
              set((state) => ({
                studentProfile: {
                  ...state.studentProfile,
                  name: profile.name ?? state.studentProfile.name,
                  age: profile.age ?? state.studentProfile.age,
                  schoolYear: profile.schoolYear ?? state.studentProfile.schoolYear,
                  schoolLevel: profile.schoolLevel ?? state.studentProfile.schoolLevel,
                  gradeLevel: profile.gradeLevel ?? state.studentProfile.gradeLevel,
                  learningGoals: profile.learningGoals ?? state.studentProfile.learningGoals,
                  fontSize: accessibility.fontSize ?? state.studentProfile.fontSize,
                  highContrast: accessibility.highContrast ?? state.studentProfile.highContrast,
                  dyslexiaFont: accessibility.dyslexiaFont ?? state.studentProfile.dyslexiaFont,
                  voiceEnabled: accessibility.voiceEnabled ?? state.studentProfile.voiceEnabled,
                  simplifiedLanguage: accessibility.simplifiedLanguage ?? state.studentProfile.simplifiedLanguage,
                  adhdMode: accessibility.adhdMode ?? state.studentProfile.adhdMode,
                },
              }));
            }
          }

          set({ lastSyncedAt: new Date(), pendingSync: false });
        } catch (error) {
          console.error('Settings load failed:', error);
        }
      },
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
          // Sync main progress
          await fetch('/api/progress', {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              xp: state.xp,
              level: state.level,
              streak: state.streak,
              totalStudyMinutes: state.totalStudyMinutes,
              questionsAsked: state.questionsAsked,
              masteries: state.masteries,
              achievements: state.achievements,
            }),
          });

          // Sync recent sessions
          const unsyncedSessions = state.sessionHistory.filter(
            (s) => s.endedAt && !s.id.startsWith('synced-')
          ).slice(0, 10);

          for (const session of unsyncedSessions) {
            await fetch('/api/progress/sessions', {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify(session),
            });
          }

          set({ lastSyncedAt: new Date(), pendingSync: false });
        } catch (error) {
          console.error('Progress sync failed:', error);
        }
      },

      loadFromServer: async () => {
        try {
          const [progressRes, sessionsRes] = await Promise.all([
            fetch('/api/progress'),
            fetch('/api/progress/sessions?limit=20'),
          ]);

          if (progressRes.ok) {
            const data = await progressRes.json();
            set((state) => ({
              xp: data.xp ?? state.xp,
              level: data.level ?? state.level,
              streak: data.streak ?? state.streak,
              masteries: data.masteries ?? state.masteries,
              achievements: data.achievements ?? state.achievements,
              totalStudyMinutes: data.totalStudyMinutes ?? state.totalStudyMinutes,
              questionsAsked: data.questionsAsked ?? state.questionsAsked,
            }));
          }

          if (sessionsRes.ok) {
            const sessions = await sessionsRes.json();
            if (Array.isArray(sessions)) {
              // Calculate sessionsThisWeek from DB sessions
              const weekAgo = new Date(Date.now() - 7 * 24 * 60 * 60 * 1000);
              const recentCount = sessions.filter(
                (s: StudySession) => new Date(s.startedAt) > weekAgo
              ).length;

              set({
                sessionHistory: sessions.map((s: StudySession) => ({
                  ...s,
                  id: `synced-${s.id}`, // Mark as synced
                  startedAt: new Date(s.startedAt),
                  endedAt: s.endedAt ? new Date(s.endedAt) : undefined,
                })),
                sessionsThisWeek: recentCount,
              });
            }
          }

          set({ lastSyncedAt: new Date(), pendingSync: false });
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
  // Sync state
  lastSyncedAt: Date | null;
  pendingSync: boolean;
  // Actions
  createConversation: (maestroId?: string) => Promise<string>;
  addMessage: (conversationId: string, message: Omit<ChatMessage, 'id' | 'timestamp'>) => Promise<void>;
  setCurrentConversation: (id: string | null) => void;
  deleteConversation: (id: string) => Promise<void>;
  clearConversations: () => void;
  // Sync actions
  syncToServer: () => Promise<void>;
  loadFromServer: () => Promise<void>;
}

export const useConversationStore = create<ConversationState>()(
  persist(
    (set) => ({
      conversations: [],
      currentConversationId: null,
      lastSyncedAt: null,
      pendingSync: false,

      createConversation: async (maestroId) => {
        const tempId = crypto.randomUUID();
        const conversation: Conversation = {
          id: tempId,
          title: 'New Conversation',
          messages: [],
          maestroId,
          createdAt: new Date(),
          updatedAt: new Date(),
        };

        // Optimistic update
        set((state) => ({
          conversations: [conversation, ...state.conversations],
          currentConversationId: tempId,
        }));

        // Sync to server
        try {
          const response = await fetch('/api/conversations', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ maestroId }),
          });

          if (response.ok) {
            const serverConv = await response.json();
            // Update with server ID
            set((state) => ({
              conversations: state.conversations.map((c) =>
                c.id === tempId ? { ...c, id: serverConv.id } : c
              ),
              currentConversationId:
                state.currentConversationId === tempId
                  ? serverConv.id
                  : state.currentConversationId,
            }));
            return serverConv.id;
          }
        } catch (error) {
          console.error('Failed to create conversation on server:', error);
        }

        return tempId;
      },

      addMessage: async (conversationId, message) => {
        const messageWithId = {
          ...message,
          id: crypto.randomUUID(),
          timestamp: new Date(),
        };

        // Optimistic update
        set((state) => ({
          conversations: state.conversations.map((conv) =>
            conv.id === conversationId
              ? {
                  ...conv,
                  messages: [...conv.messages, messageWithId],
                  updatedAt: new Date(),
                  title:
                    conv.messages.length === 0 && message.role === 'user'
                      ? message.content.slice(0, 50)
                      : conv.title,
                }
              : conv
          ),
          pendingSync: true,
        }));

        // Sync to server
        try {
          await fetch(`/api/conversations/${conversationId}/messages`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              role: message.role,
              content: message.content,
              toolCalls: (message as { toolCalls?: unknown }).toolCalls,
            }),
          });
        } catch (error) {
          console.error('Failed to add message to server:', error);
        }
      },

      setCurrentConversation: (id) => set({ currentConversationId: id }),

      deleteConversation: async (id) => {
        // Optimistic update
        set((state) => ({
          conversations: state.conversations.filter((c) => c.id !== id),
          currentConversationId:
            state.currentConversationId === id
              ? null
              : state.currentConversationId,
        }));

        // Sync to server
        try {
          await fetch(`/api/conversations/${id}`, { method: 'DELETE' });
        } catch (error) {
          console.error('Failed to delete conversation on server:', error);
        }
      },

      clearConversations: () =>
        set({ conversations: [], currentConversationId: null }),

      syncToServer: async () => {
        // Conversations are synced immediately on each action
        set({ lastSyncedAt: new Date(), pendingSync: false });
      },

      loadFromServer: async () => {
        try {
          const response = await fetch('/api/conversations?limit=50');
          if (response.ok) {
            const conversations = await response.json();
            set({
              conversations: conversations.map((c: {
                id: string;
                title: string;
                maestroId: string;
                createdAt: string;
                updatedAt: string;
                lastMessage?: string;
              }) => ({
                ...c,
                messages: [], // Messages are loaded on-demand
                createdAt: new Date(c.createdAt),
                updatedAt: new Date(c.updatedAt),
              })),
              lastSyncedAt: new Date(),
              pendingSync: false,
            });
          }
        } catch (error) {
          console.error('Conversations load failed:', error);
        }
      },
    }),
    { name: 'convergio-conversations' }
  )
);

// === LEARNINGS STORE ===

interface Learning {
  id: string;
  category: 'preference' | 'strength' | 'weakness' | 'interest' | 'style';
  insight: string;
  maestroId?: string;
  subject?: string;
  confidence: number;
  occurrences: number;
  createdAt: Date;
  updatedAt: Date;
}

interface LearningsState {
  learnings: Learning[];
  lastSyncedAt: Date | null;
  // Actions
  addLearning: (learning: Omit<Learning, 'id' | 'occurrences' | 'createdAt' | 'updatedAt'>) => Promise<void>;
  removeLearning: (id: string) => Promise<void>;
  getLearningsByCategory: (category: Learning['category']) => Learning[];
  // Sync
  loadFromServer: () => Promise<void>;
}

export const useLearningsStore = create<LearningsState>()(
  persist(
    (set, get) => ({
      learnings: [],
      lastSyncedAt: null,

      addLearning: async (learning) => {
        try {
          const response = await fetch('/api/learnings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(learning),
          });

          if (response.ok) {
            const serverLearning = await response.json();
            set((state) => {
              // Check if this was a reinforcement
              if (serverLearning.reinforced) {
                return {
                  learnings: state.learnings.map((l) =>
                    l.id === serverLearning.id
                      ? { ...l, confidence: serverLearning.confidence, occurrences: serverLearning.occurrences }
                      : l
                  ),
                };
              }
              // New learning
              return {
                learnings: [
                  {
                    ...serverLearning,
                    createdAt: new Date(serverLearning.createdAt),
                    updatedAt: new Date(serverLearning.updatedAt),
                  },
                  ...state.learnings,
                ],
              };
            });
          }
        } catch (error) {
          console.error('Failed to add learning:', error);
        }
      },

      removeLearning: async (id) => {
        set((state) => ({
          learnings: state.learnings.filter((l) => l.id !== id),
        }));

        try {
          await fetch(`/api/learnings?id=${id}`, { method: 'DELETE' });
        } catch (error) {
          console.error('Failed to remove learning:', error);
        }
      },

      getLearningsByCategory: (category) => {
        return get().learnings.filter((l) => l.category === category);
      },

      loadFromServer: async () => {
        try {
          const response = await fetch('/api/learnings');
          if (response.ok) {
            const learnings = await response.json();
            set({
              learnings: learnings.map((l: Learning & { createdAt: string; updatedAt: string }) => ({
                ...l,
                createdAt: new Date(l.createdAt),
                updatedAt: new Date(l.updatedAt),
              })),
              lastSyncedAt: new Date(),
            });
          }
        } catch (error) {
          console.error('Learnings load failed:', error);
        }
      },
    }),
    { name: 'convergio-learnings' }
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

// === SYNC HOOK ===

export async function initializeStores() {
  // Ensure user exists (creates cookie if needed)
  await fetch('/api/user');

  // Load data from server
  await Promise.all([
    useSettingsStore.getState().loadFromServer(),
    useProgressStore.getState().loadFromServer(),
    useConversationStore.getState().loadFromServer(),
    useLearningsStore.getState().loadFromServer(),
  ]);
}

export function setupAutoSync(intervalMs = 30000) {
  // Auto-sync every 30 seconds if there are pending changes
  return setInterval(async () => {
    const settings = useSettingsStore.getState();
    const progress = useProgressStore.getState();

    if (settings.pendingSync) {
      await settings.syncToServer();
    }
    if (progress.pendingSync) {
      await progress.syncToServer();
    }
  }, intervalMs);
}
