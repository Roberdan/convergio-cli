// Convergio Cost API Client

export interface CostByService {
  service_name: string;
  cost: number;
  currency: string;
}

export interface DailyCost {
  date: string;
  cost: number;
  currency: string;
}

export interface CostSummary {
  subscription_id: string;
  subscription_name: string;
  period_start: string;
  period_end: string;
  total_cost: number;
  currency: string;
  costs_by_service: CostByService[];
  daily_costs: DailyCost[];
}

export interface CostForecast {
  subscription_id: string;
  forecast_period_end: string;
  estimated_total: number;
  currency: string;
}

export interface ConvergioCostClientOptions {
  baseURL?: string;
  timeout?: number;
}

export class ConvergioCostClient {
  private readonly baseURL: string;
  private readonly timeout: number;

  constructor(options: ConvergioCostClientOptions = {}) {
    this.baseURL = options.baseURL ?? "https://api.convergio.ai";
    this.timeout = options.timeout ?? 30000;
  }

  private async fetch<T>(path: string, params?: Record<string, string>): Promise<T> {
    const url = new URL(path, this.baseURL);
    if (params) {
      Object.entries(params).forEach(([key, value]) => {
        url.searchParams.append(key, value);
      });
    }

    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), this.timeout);

    try {
      const response = await fetch(url.toString(), {
        method: "GET",
        headers: {
          Accept: "application/json",
        },
        signal: controller.signal,
      });

      if (!response.ok) {
        throw new Error(`HTTP error: ${response.status} ${response.statusText}`);
      }

      return await response.json();
    } finally {
      clearTimeout(timeoutId);
    }
  }

  async getCosts(days: number = 30): Promise<CostSummary> {
    return this.fetch<CostSummary>("/api/v1/costs", { days: days.toString() });
  }

  async getForecast(): Promise<CostForecast> {
    return this.fetch<CostForecast>("/api/v1/costs/forecast");
  }

  async getCurrentMonthCosts(): Promise<CostSummary> {
    return this.fetch<CostSummary>("/api/v1/costs/current-month");
  }
}

// Default export for convenience
export default ConvergioCostClient;
