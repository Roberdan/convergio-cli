import Foundation

// MARK: - Models

struct CostByService: Codable, Identifiable {
    var id: String { serviceName }
    let serviceName: String
    let cost: Double
    let currency: String

    enum CodingKeys: String, CodingKey {
        case serviceName = "service_name"
        case cost
        case currency
    }
}

struct DailyCost: Codable, Identifiable {
    var id: String { date }
    let date: String
    let cost: Double
    let currency: String
}

struct CostSummary: Codable {
    let subscriptionId: String
    let subscriptionName: String
    let periodStart: String
    let periodEnd: String
    let totalCost: Double
    let currency: String
    let costsByService: [CostByService]
    let dailyCosts: [DailyCost]

    enum CodingKeys: String, CodingKey {
        case subscriptionId = "subscription_id"
        case subscriptionName = "subscription_name"
        case periodStart = "period_start"
        case periodEnd = "period_end"
        case totalCost = "total_cost"
        case currency
        case costsByService = "costs_by_service"
        case dailyCosts = "daily_costs"
    }
}

struct CostForecast: Codable {
    let subscriptionId: String
    let forecastPeriodEnd: String
    let estimatedTotal: Double
    let currency: String

    enum CodingKeys: String, CodingKey {
        case subscriptionId = "subscription_id"
        case forecastPeriodEnd = "forecast_period_end"
        case estimatedTotal = "estimated_total"
        case currency
    }
}

// MARK: - Service

@MainActor
final class AzureCostService: ObservableObject {
    static let shared = AzureCostService()

    @Published private(set) var costSummary: CostSummary?
    @Published private(set) var forecast: CostForecast?
    @Published private(set) var isLoading = false
    @Published private(set) var error: Error?

    private let baseURL: URL
    private let session: URLSession

    init(baseURL: String = "https://api.convergio.ai") {
        self.baseURL = URL(string: baseURL)!

        let config = URLSessionConfiguration.default
        config.timeoutIntervalForRequest = 30
        config.waitsForConnectivity = true
        self.session = URLSession(configuration: config)
    }

    func fetchCosts(days: Int = 30) async {
        isLoading = true
        error = nil

        do {
            let url = baseURL.appendingPathComponent("/api/v1/costs")
            var components = URLComponents(url: url, resolvingAgainstBaseURL: true)!
            components.queryItems = [URLQueryItem(name: "days", value: String(days))]

            let (data, response) = try await session.data(from: components.url!)

            guard let httpResponse = response as? HTTPURLResponse,
                  (200...299).contains(httpResponse.statusCode) else {
                throw URLError(.badServerResponse)
            }

            let decoder = JSONDecoder()
            costSummary = try decoder.decode(CostSummary.self, from: data)
        } catch {
            self.error = error
        }

        isLoading = false
    }

    func fetchForecast() async {
        do {
            let url = baseURL.appendingPathComponent("/api/v1/costs/forecast")
            let (data, response) = try await session.data(from: url)

            guard let httpResponse = response as? HTTPURLResponse,
                  (200...299).contains(httpResponse.statusCode) else {
                throw URLError(.badServerResponse)
            }

            let decoder = JSONDecoder()
            forecast = try decoder.decode(CostForecast.self, from: data)
        } catch {
            self.error = error
        }
    }

    func refresh() async {
        await fetchCosts()
        await fetchForecast()
    }
}
