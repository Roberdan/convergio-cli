#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// Paths - use relative paths from script location
const sourceDir = path.join(__dirname, '../src/agents/definitions/education');
const outputFile = process.env.EDU_OUTPUT || path.join(__dirname, '../../ConvergioEdu/src/data/maestri-full.ts');

// Files to exclude
const excludeFiles = [
  'SAFETY_AND_INCLUSIVITY_GUIDELINES.md',
  'test-education-agent.md',
  'ali-principal.md'
];

// Subject mapping based on filename
const subjectMapping = {
  'euclide-matematica': 'mathematics',
  'erodoto-storia': 'history',
  'darwin-scienze': 'science',
  'curie-chimica': 'chemistry',
  'feynman-fisica': 'physics',
  'galileo-astronomia': 'astronomy',
  'humboldt-geografia': 'geography',
  'ippocrate-corpo': 'physical-education',
  'leonardo-arte': 'art',
  'lovelace-informatica': 'computer-science',
  'manzoni-italiano': 'italian',
  'mozart-musica': 'music',
  'shakespeare-inglese': 'english',
  'smith-economia': 'economics',
  'socrate-filosofia': 'philosophy',
  'cicerone-civica': 'civic-education',
  'chris-storytelling': 'storytelling'
};

// Color mapping based on subject
const colorMapping = {
  'mathematics': '#2980B9',
  'history': '#9B59B6',
  'science': '#1ABC9C',
  'chemistry': '#E74C3C',
  'physics': '#3498DB',
  'astronomy': '#34495E',
  'geography': '#16A085',
  'physical-education': '#27AE60',
  'art': '#E67E22',
  'computer-science': '#8E44AD',
  'italian': '#C0392B',
  'music': '#F39C12',
  'english': '#9B59B6',
  'economics': '#D35400',
  'philosophy': '#7F8C8D',
  'civic-education': '#2C3E50',
  'storytelling': '#E91E63'
};

// Parse YAML frontmatter
function parseFrontmatter(content) {
  const frontmatterRegex = /^---\n([\s\S]*?)\n---/;
  const match = content.match(frontmatterRegex);

  if (!match) return { frontmatter: {}, content: content };

  const frontmatterText = match[1];
  const remainingContent = content.substring(match[0].length).trim();

  const frontmatter = {};
  const lines = frontmatterText.split('\n');

  for (const line of lines) {
    if (line.includes(':')) {
      const colonIndex = line.indexOf(':');
      const key = line.substring(0, colonIndex).trim();
      let value = line.substring(colonIndex + 1).trim();

      // Parse arrays
      if (value.startsWith('[') && value.endsWith(']')) {
        value = JSON.parse(value.replace(/"/g, '"'));
      }
      // Remove quotes
      else if ((value.startsWith('"') && value.endsWith('"')) ||
               (value.startsWith("'") && value.endsWith("'"))) {
        value = value.slice(1, -1);
      }

      frontmatter[key] = value;
    }
  }

  return { frontmatter, content: remainingContent };
}

// Extract display name from content
function extractDisplayName(content) {
  const match = content.match(/You are \*\*([^*]+)\*\*/);
  return match ? match[1] : '';
}

// Extract greeting from content
function extractGreeting(content, displayName) {
  // Try to find a greeting in example interactions
  const greetingMatch = content.match(/Student:.*?\n\n([A-Z][^:]+):/);
  if (greetingMatch) {
    return `Ciao! Sono ${displayName}. Come posso aiutarti oggi?`;
  }
  return `Benvenuto! Sono ${displayName}, il tuo maestro. Cosa vorresti imparare oggi?`;
}

// Escape special characters for TypeScript string
function escapeForTypeScript(str) {
  return str
    .replace(/\\/g, '\\\\')
    .replace(/`/g, '\\`')
    .replace(/\$/g, '\\$');
}

// Main function
async function exportMaestri() {
  console.log('Starting maestri export...\n');

  // Read all markdown files
  const files = fs.readdirSync(sourceDir)
    .filter(file => file.endsWith('.md'))
    .filter(file => !excludeFiles.includes(file));

  console.log(`Found ${files.length} maestri files to process.\n`);

  const maestri = [];

  for (const file of files) {
    const filePath = path.join(sourceDir, file);
    const content = fs.readFileSync(filePath, 'utf-8');
    const { frontmatter, content: markdownContent } = parseFrontmatter(content);

    const id = file.replace('.md', '');
    const displayName = extractDisplayName(markdownContent);
    const subject = subjectMapping[id] || 'general';
    const color = frontmatter.color || colorMapping[subject] || '#95A5A6';
    const greeting = extractGreeting(markdownContent, displayName);

    const maestro = {
      id,
      name: frontmatter.name || id,
      displayName: displayName || frontmatter.name || id,
      subject,
      tools: frontmatter.tools || [],
      systemPrompt: markdownContent,
      avatar: `/maestri/${id}.png`,
      color,
      greeting
    };

    maestri.push(maestro);
    console.log(`✓ Processed: ${maestro.displayName} (${id})`);
  }

  console.log(`\n✓ Processed ${maestri.length} maestri.\n`);

  // Sort by subject then by name
  maestri.sort((a, b) => {
    if (a.subject === b.subject) {
      return a.displayName.localeCompare(b.displayName);
    }
    return a.subject.localeCompare(b.subject);
  });

  // Read safety guidelines
  const safetyPath = path.join(sourceDir, 'SAFETY_AND_INCLUSIVITY_GUIDELINES.md');
  const safetyContent = fs.readFileSync(safetyPath, 'utf-8');

  // Generate TypeScript file
  let tsContent = `/**
 * Convergio Education Maestri - Full Export
 * Auto-generated from CLI markdown files
 * Generated: ${new Date().toISOString()}
 *
 * Source: /Users/roberdan/GitHub/ConvergioCLI/src/agents/definitions/education/
 * DO NOT EDIT MANUALLY - Regenerate using scripts/export-maestri.js
 */

export interface MaestroFull {
  id: string;           // from filename, e.g., "euclide-matematica"
  name: string;         // from frontmatter
  displayName: string;  // Human readable, e.g., "Euclide"
  subject: string;      // e.g., "mathematics", "history", etc.
  tools: string[];      // from frontmatter
  systemPrompt: string; // full markdown content after frontmatter
  avatar: string;       // \`/maestri/\${id}.png\`
  color: string;        // subject-based color
  greeting: string;     // extract first greeting or generate one
}

// Safety and Inclusivity Guidelines
export const SAFETY_GUIDELINES: string = \`${escapeForTypeScript(safetyContent)}\`;

// All Education Maestri
export const MAESTRI: MaestroFull[] = [
`;

  // Add each maestro
  for (let i = 0; i < maestri.length; i++) {
    const m = maestri[i];
    tsContent += `  {
    id: '${m.id}',
    name: '${m.name}',
    displayName: '${m.displayName}',
    subject: '${m.subject}',
    tools: ${JSON.stringify(m.tools)},
    systemPrompt: \`${escapeForTypeScript(m.systemPrompt)}\`,
    avatar: '${m.avatar}',
    color: '${m.color}',
    greeting: \`${escapeForTypeScript(m.greeting)}\`
  }${i < maestri.length - 1 ? ',' : ''}
`;
  }

  tsContent += `];

// Helper functions
export function getMaestroById(id: string): MaestroFull | undefined {
  return MAESTRI.find(m => m.id === id);
}

export function getMaestriBySubject(subject: string): MaestroFull[] {
  return MAESTRI.filter(m => m.subject === subject);
}

export function getAllSubjects(): string[] {
  return Array.from(new Set(MAESTRI.map(m => m.subject))).sort();
}

// Subject display names
export const SUBJECT_NAMES: Record<string, string> = {
  'mathematics': 'Matematica',
  'history': 'Storia',
  'science': 'Scienze',
  'chemistry': 'Chimica',
  'physics': 'Fisica',
  'astronomy': 'Astronomia',
  'geography': 'Geografia',
  'physical-education': 'Educazione Fisica',
  'art': 'Arte',
  'computer-science': 'Informatica',
  'italian': 'Italiano',
  'music': 'Musica',
  'english': 'Inglese',
  'economics': 'Economia',
  'philosophy': 'Filosofia',
  'civic-education': 'Educazione Civica',
  'storytelling': 'Storytelling'
};
`;

  // Ensure output directory exists
  const outputDir = path.dirname(outputFile);
  if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir, { recursive: true });
    console.log(`Created output directory: ${outputDir}\n`);
  }

  // Write the file
  fs.writeFileSync(outputFile, tsContent, 'utf-8');

  console.log(`✓ Successfully generated TypeScript file:\n  ${outputFile}\n`);
  console.log(`Statistics:`);
  console.log(`  - Maestri exported: ${maestri.length}`);
  console.log(`  - Subjects covered: ${getAllSubjects(maestri).length}`);
  console.log(`  - Total file size: ${(tsContent.length / 1024).toFixed(2)} KB\n`);
}

function getAllSubjects(maestri) {
  return Array.from(new Set(maestri.map(m => m.subject))).sort();
}

// Run the export
exportMaestri().catch(err => {
  console.error('Error exporting maestri:', err);
  process.exit(1);
});
