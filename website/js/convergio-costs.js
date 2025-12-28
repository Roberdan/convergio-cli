/**
 * Convergio Azure Cost API Client
 * Lightweight browser client for cost monitoring
 */

const ConvergioCosts = {
  baseURL: "https://api.convergio.ai",

  async getCosts(days = 30) {
    const response = await fetch(`${this.baseURL}/api/v1/costs?days=${days}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  },

  async getForecast() {
    const response = await fetch(`${this.baseURL}/api/v1/costs/forecast`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  },

  async getCurrentMonth() {
    const response = await fetch(`${this.baseURL}/api/v1/costs/current-month`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  },

  formatCurrency(amount, currency = "USD") {
    return new Intl.NumberFormat("en-US", {
      style: "currency",
      currency: currency,
    }).format(amount);
  },
};

// Export for module usage
if (typeof module !== "undefined" && module.exports) {
  module.exports = ConvergioCosts;
}
