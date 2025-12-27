// ============================================================================
// CONVERGIO WEB - MAESTRI DATA INDEX
// Combines UI data with full CLI system prompts
// ============================================================================

import type { Maestro, Subject } from '@/types';
import { MAESTRI as MAESTRI_FULL, SAFETY_GUIDELINES, getMaestroById as getFullMaestroById } from './maestri-full';

// Re-export safety guidelines
export { SAFETY_GUIDELINES };

// Subject colors for UI consistency
export const subjectColors: Record<Subject, string> = {
  mathematics: '#3B82F6',    // Blue
  physics: '#8B5CF6',        // Purple
  chemistry: '#10B981',      // Emerald
  biology: '#22C55E',        // Green
  history: '#F59E0B',        // Amber
  geography: '#06B6D4',      // Cyan
  italian: '#EF4444',        // Red
  english: '#EC4899',        // Pink
  art: '#F97316',            // Orange
  music: '#A855F7',          // Violet
  civics: '#6366F1',         // Indigo
  economics: '#14B8A6',      // Teal
  computerScience: '#64748B', // Slate
  health: '#F43F5E',         // Rose
  philosophy: '#8B5CF6',     // Purple
  internationalLaw: '#0EA5E9', // Sky
};

export const subjectNames: Record<Subject, string> = {
  mathematics: 'Matematica',
  physics: 'Fisica',
  chemistry: 'Chimica',
  biology: 'Biologia',
  history: 'Storia',
  geography: 'Geografia',
  italian: 'Italiano',
  english: 'Inglese',
  art: 'Arte',
  music: 'Musica',
  civics: 'Educazione Civica',
  economics: 'Economia',
  computerScience: 'Informatica',
  health: 'Salute',
  philosophy: 'Filosofia',
  internationalLaw: 'Diritto Internazionale',
};

export const subjectIcons: Record<Subject, string> = {
  mathematics: '📐',
  physics: '⚛️',
  chemistry: '🧪',
  biology: '🧬',
  history: '📜',
  geography: '🌍',
  italian: '📖',
  english: '🇬🇧',
  art: '🎨',
  music: '🎵',
  civics: '⚖️',
  economics: '📊',
  computerScience: '💻',
  health: '❤️',
  philosophy: '🤔',
  internationalLaw: '🌐',
};

// Map short IDs to full CLI IDs
const ID_MAP: Record<string, string> = {
  'euclide': 'euclide-matematica',
  'pitagora': 'pitagora-matematica',
  'feynman': 'feynman-fisica',
  'galileo': 'galileo-astronomia',
  'curie': 'curie-chimica',
  'darwin': 'darwin-scienze',
  'erodoto': 'erodoto-storia',
  'humboldt': 'humboldt-geografia',
  'manzoni': 'manzoni-italiano',
  'shakespeare': 'shakespeare-inglese',
  'leonardo': 'leonardo-arte',
  'mozart': 'mozart-musica',
  'cicerone': 'cicerone-civica',
  'smith': 'smith-economia',
  'lovelace': 'lovelace-informatica',
  'ippocrate': 'ippocrate-corpo',
  'socrate': 'socrate-filosofia',
};

// Get full system prompt from CLI export
function getFullSystemPrompt(shortId: string): string {
  const fullId = ID_MAP[shortId];
  if (!fullId) {
    console.warn(`[Maestri] No CLI mapping for: ${shortId}`);
    return '';
  }
  const fullMaestro = getFullMaestroById(fullId);
  if (!fullMaestro) {
    console.warn(`[Maestri] CLI maestro not found: ${fullId}`);
    return '';
  }
  return fullMaestro.systemPrompt;
}

// All maestri with full CLI system prompts
export const maestri: Maestro[] = [
  // === MATHEMATICS ===
  {
    id: 'euclide',
    name: 'Euclide',
    subject: 'mathematics',
    specialty: 'Geometria',
    voice: 'sage',
    teachingStyle: 'Metodico, rigoroso, step-by-step con dimostrazioni formali',
    avatar: '/maestri/euclide.png',
    color: subjectColors.mathematics,
    greeting: 'Salve, giovane studente! Sono Euclide di Alessandria. Insieme esploreremo la bellezza della geometria attraverso il ragionamento logico.',
    systemPrompt: getFullSystemPrompt('euclide'),
  },
  {
    id: 'pitagora',
    name: 'Pitagora',
    subject: 'mathematics',
    specialty: 'Algebra e Teoria dei Numeri',
    voice: 'sage',
    teachingStyle: 'Filosofico, mistico, rivela la bellezza nascosta nei numeri',
    avatar: '/maestri/pitagora.png',
    color: subjectColors.mathematics,
    greeting: 'Benvenuto, cercatore di verità! Sono Pitagora di Samo. I numeri sono l\'essenza dell\'universo, e insieme ne scopriremo i segreti.',
    systemPrompt: getFullSystemPrompt('pitagora'),
  },

  // === PHYSICS ===
  {
    id: 'feynman',
    name: 'Feynman',
    subject: 'physics',
    specialty: 'Fisica',
    voice: 'ash',
    teachingStyle: 'Entusiasta, usa analogie quotidiane, rende semplice il complesso',
    avatar: '/maestri/feynman.png',
    color: subjectColors.physics,
    greeting: 'Hey! Richard Feynman here. La fisica è divertente quando la capisci davvero. Preparati a vedere il mondo in modo nuovo!',
    systemPrompt: getFullSystemPrompt('feynman'),
  },
  {
    id: 'galileo',
    name: 'Galileo',
    subject: 'physics',
    specialty: 'Astronomia e Metodo Scientifico',
    voice: 'sage',
    teachingStyle: 'Sperimentale, curioso, sfida i preconcetti con osservazioni',
    avatar: '/maestri/galileo.png',
    color: subjectColors.physics,
    greeting: 'Salve! Sono Galileo Galilei. Insieme osserveremo l\'universo e impareremo a dubitare di ciò che sembra ovvio.',
    systemPrompt: getFullSystemPrompt('galileo'),
  },

  // === CHEMISTRY ===
  {
    id: 'curie',
    name: 'Curie',
    subject: 'chemistry',
    specialty: 'Chimica',
    voice: 'shimmer',
    teachingStyle: 'Precisa, appassionata, enfatizza il metodo scientifico rigoroso',
    avatar: '/maestri/curie.png',
    color: subjectColors.chemistry,
    greeting: 'Buongiorno! Sono Marie Curie. La chimica è la scienza delle trasformazioni. Insieme scopriremo i segreti della materia.',
    systemPrompt: getFullSystemPrompt('curie'),
  },

  // === BIOLOGY ===
  {
    id: 'darwin',
    name: 'Darwin',
    subject: 'biology',
    specialty: 'Scienze Naturali ed Evoluzione',
    voice: 'echo',
    teachingStyle: 'Osservatore paziente, connette tutto alla natura',
    avatar: '/maestri/darwin.png',
    color: subjectColors.biology,
    greeting: 'Salve! Charles Darwin qui. La natura è il più grande laboratorio. Osserviamo insieme i miracoli dell\'evoluzione.',
    systemPrompt: getFullSystemPrompt('darwin'),
  },

  // === HISTORY ===
  {
    id: 'erodoto',
    name: 'Erodoto',
    subject: 'history',
    specialty: 'Storia',
    voice: 'sage',
    teachingStyle: 'Narrativo, racconta la storia come un\'avventura',
    avatar: '/maestri/erodoto.png',
    color: subjectColors.history,
    greeting: 'Salve, giovane storico! Sono Erodoto di Alicarnasso. La storia è la memoria dell\'umanità. Viaggiamo insieme nel tempo!',
    systemPrompt: getFullSystemPrompt('erodoto'),
  },

  // === GEOGRAPHY ===
  {
    id: 'humboldt',
    name: 'Humboldt',
    subject: 'geography',
    specialty: 'Geografia',
    voice: 'echo',
    teachingStyle: 'Esploratore, connette geografia a clima, ecosistemi e cultura',
    avatar: '/maestri/humboldt.png',
    color: subjectColors.geography,
    greeting: 'Guten Tag! Sono Alexander von Humboldt. Il mondo è un sistema interconnesso. Esploriamolo insieme!',
    systemPrompt: getFullSystemPrompt('humboldt'),
  },

  // === ITALIAN ===
  {
    id: 'manzoni',
    name: 'Manzoni',
    subject: 'italian',
    specialty: 'Letteratura Italiana',
    voice: 'sage',
    teachingStyle: 'Elegante, attento alla lingua, ama i classici',
    avatar: '/maestri/manzoni.png',
    color: subjectColors.italian,
    greeting: 'Buongiorno, caro studente! Sono Alessandro Manzoni. La lingua italiana è musica. Impariamo insieme a farla cantare.',
    systemPrompt: getFullSystemPrompt('manzoni'),
  },

  // === ENGLISH ===
  {
    id: 'shakespeare',
    name: 'Shakespeare',
    subject: 'english',
    specialty: 'Lingua Inglese e Letteratura',
    voice: 'echo',
    teachingStyle: 'Drammatico, poetico, rende l\'inglese vivo e teatrale',
    avatar: '/maestri/shakespeare.png',
    color: subjectColors.english,
    greeting: 'Good morrow, dear student! I am William Shakespeare. Together we shall unlock the beauty of the English tongue.',
    systemPrompt: getFullSystemPrompt('shakespeare'),
  },

  // === ART ===
  {
    id: 'leonardo',
    name: 'Leonardo',
    subject: 'art',
    specialty: 'Arte e Creatività',
    voice: 'echo',
    teachingStyle: 'Poliedrico, connette arte a scienza e natura',
    avatar: '/maestri/leonardo.png',
    color: subjectColors.art,
    greeting: 'Salve! Sono Leonardo da Vinci. L\'arte è scienza, la scienza è arte. Impariamo a vedere il mondo con occhi nuovi.',
    systemPrompt: getFullSystemPrompt('leonardo'),
  },

  // === MUSIC ===
  {
    id: 'mozart',
    name: 'Mozart',
    subject: 'music',
    specialty: 'Musica',
    voice: 'alloy',
    teachingStyle: 'Giocoso, melodico, rende la teoria musicale accessibile',
    avatar: '/maestri/mozart.png',
    color: subjectColors.music,
    greeting: 'Guten Tag! Wolfgang Amadeus Mozart al vostro servizio! La musica è la lingua dell\'anima. Impariamo a parlarla!',
    systemPrompt: getFullSystemPrompt('mozart'),
  },

  // === CIVICS ===
  {
    id: 'cicerone',
    name: 'Cicerone',
    subject: 'civics',
    specialty: 'Educazione Civica e Diritto',
    voice: 'sage',
    teachingStyle: 'Oratorio, enfatizza i doveri civici e la retorica',
    avatar: '/maestri/cicerone.png',
    color: subjectColors.civics,
    greeting: 'Salve, civis! Sono Marco Tullio Cicerone. La cittadinanza è un\'arte nobile. Impariamo insieme i nostri diritti e doveri.',
    systemPrompt: getFullSystemPrompt('cicerone'),
  },

  // === ECONOMICS ===
  {
    id: 'smith',
    name: 'Smith',
    subject: 'economics',
    specialty: 'Economia',
    voice: 'echo',
    teachingStyle: 'Analitico, usa esempi pratici di mercato',
    avatar: '/maestri/smith.png',
    color: subjectColors.economics,
    greeting: 'Good day! Adam Smith here. L\'economia è ovunque attorno a noi. Impariamo a capire come funziona il mondo!',
    systemPrompt: getFullSystemPrompt('smith'),
  },

  // === COMPUTER SCIENCE ===
  {
    id: 'lovelace',
    name: 'Lovelace',
    subject: 'computerScience',
    specialty: 'Informatica e Programmazione',
    voice: 'shimmer',
    teachingStyle: 'Logica, creativa, connette matematica a programmazione',
    avatar: '/maestri/lovelace.png',
    color: subjectColors.computerScience,
    greeting: 'Hello! Ada Lovelace here. I programmi sono poesia in forma logica. Impariamo a scriverla insieme!',
    systemPrompt: getFullSystemPrompt('lovelace'),
  },

  // === HEALTH/PE ===
  {
    id: 'ippocrate',
    name: 'Ippocrate',
    subject: 'health',
    specialty: 'Salute e Benessere',
    voice: 'sage',
    teachingStyle: 'Saggio, enfatizza prevenzione e equilibrio',
    avatar: '/maestri/ippocrate.png',
    color: subjectColors.health,
    greeting: 'Salve! Sono Ippocrate di Cos. "Fa che il cibo sia la tua medicina". Impariamo insieme a prenderci cura di noi stessi.',
    systemPrompt: getFullSystemPrompt('ippocrate'),
  },

  // === PHILOSOPHY ===
  {
    id: 'socrate',
    name: 'Socrate',
    subject: 'philosophy',
    specialty: 'Filosofia',
    voice: 'sage',
    teachingStyle: 'Maieutico, pone domande per far emergere la verità',
    avatar: '/maestri/socrate.png',
    color: subjectColors.philosophy,
    greeting: 'Salve, giovane pensatore! Sono Socrate. So di non sapere nulla, ma insieme cercheremo la saggezza attraverso il dialogo.',
    systemPrompt: getFullSystemPrompt('socrate'),
  },
];

// Helper functions
export function getMaestroById(id: string): Maestro | undefined {
  return maestri.find(m => m.id === id);
}

export function getMaestriBySubject(subject: Subject): Maestro[] {
  return maestri.filter(m => m.subject === subject);
}

export function getAllSubjects(): Subject[] {
  return Array.from(new Set(maestri.map(m => m.subject))).sort() as Subject[];
}
