/**
 * CONVERGIO PROVIDER-SPECIFIC MOCKS
 *
 * Header for specialized mock providers that simulate specific LLM behaviors.
 */

#ifndef CONVERGIO_PROVIDER_MOCKS_H
#define CONVERGIO_PROVIDER_MOCKS_H

#include "../mock_provider.h"

// ============================================================================
// ANTHROPIC MOCKS
// ============================================================================

/** Create mock configured like Anthropic Claude */
MockProvider* mock_anthropic_create(void);

/** Create mock configured as Claude Sonnet */
MockProvider* mock_anthropic_sonnet(void);

/** Create mock configured as Claude Haiku (fast, cheap) */
MockProvider* mock_anthropic_haiku(void);

/** Create mock configured as Claude Opus (powerful, expensive) */
MockProvider* mock_anthropic_opus(void);

/** Create mock simulating Anthropic overload errors */
MockProvider* mock_anthropic_overloaded(void);

/** Create mock simulating Anthropic auth errors */
MockProvider* mock_anthropic_auth_error(void);

// ============================================================================
// OPENAI MOCKS
// ============================================================================

/** Create mock configured like OpenAI GPT */
MockProvider* mock_openai_create(void);

/** Create mock configured as GPT-4o */
MockProvider* mock_openai_gpt4o(void);

/** Create mock configured as GPT-4o-mini (fast, cheap) */
MockProvider* mock_openai_gpt4o_mini(void);

/** Create mock configured as o1-preview (reasoning model) */
MockProvider* mock_openai_o1(void);

/** Create mock simulating OpenAI rate limit (429) */
MockProvider* mock_openai_rate_limited(void);

/** Create mock simulating OpenAI auth error (401) */
MockProvider* mock_openai_auth_error(void);

/** Create mock simulating OpenAI quota exceeded */
MockProvider* mock_openai_quota_exceeded(void);

// ============================================================================
// GEMINI MOCKS
// ============================================================================

/** Create mock configured like Google Gemini */
MockProvider* mock_gemini_create(void);

/** Create mock configured as Gemini 2.0 Flash (latest, fastest) */
MockProvider* mock_gemini_2_flash(void);

/** Create mock configured as Gemini 1.5 Pro (powerful) */
MockProvider* mock_gemini_1_5_pro(void);

/** Create mock configured as Gemini 1.5 Flash (balanced) */
MockProvider* mock_gemini_1_5_flash(void);

/** Create mock simulating Gemini free tier (strict rate limits) */
MockProvider* mock_gemini_free_tier(void);

/** Create mock simulating Gemini rate limit (429) */
MockProvider* mock_gemini_rate_limited(void);

/** Create mock simulating Gemini auth error */
MockProvider* mock_gemini_auth_error(void);

/** Create mock simulating Gemini safety filter block */
MockProvider* mock_gemini_safety_blocked(void);

#endif // CONVERGIO_PROVIDER_MOCKS_H
