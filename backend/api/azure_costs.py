from datetime import date, timedelta, datetime
import httpx
import asyncio
from azure.identity import ClientSecretCredential, AzureCliCredential
from .config import get_settings
from .models import CostByService, DailyCost, CostSummary, CostForecast


class CostCache:
    """Simple in-memory cache with TTL."""

    def __init__(self, ttl_seconds: int = 300):  # 5 min default
        self.ttl = ttl_seconds
        self._cache: dict[str, tuple[datetime, any]] = {}

    def get(self, key: str):
        if key in self._cache:
            timestamp, value = self._cache[key]
            if (datetime.now() - timestamp).total_seconds() < self.ttl:
                return value
            del self._cache[key]
        return None

    def set(self, key: str, value: any):
        self._cache[key] = (datetime.now(), value)


class AzureCostService:
    def __init__(self):
        settings = get_settings()
        self.subscription_id = settings.azure_subscription_id
        self._cache = CostCache(ttl_seconds=300)  # Cache for 5 minutes

        if settings.use_service_principal:
            self.credential = ClientSecretCredential(
                tenant_id=settings.azure_tenant_id,
                client_id=settings.azure_client_id,
                client_secret=settings.azure_client_secret,
            )
        else:
            # Use local az login credentials
            self.credential = AzureCliCredential()

        self._token = None

    async def _get_token(self) -> str:
        if self._token is None:
            token = self.credential.get_token("https://management.azure.com/.default")
            self._token = token.token
        return self._token

    async def _query_costs(self, query_body: dict, max_retries: int = 3) -> dict:
        token = await self._get_token()
        url = (
            f"https://management.azure.com/subscriptions/{self.subscription_id}"
            f"/providers/Microsoft.CostManagement/query?api-version=2023-11-01"
        )
        import asyncio

        for attempt in range(max_retries):
            async with httpx.AsyncClient() as client:
                response = await client.post(
                    url,
                    json=query_body,
                    headers={"Authorization": f"Bearer {token}"},
                    timeout=30.0,
                )
                if response.status_code == 429:
                    retry_after = int(response.headers.get("Retry-After", 60))
                    if attempt < max_retries - 1:
                        await asyncio.sleep(retry_after)
                        continue
                response.raise_for_status()
                return response.json()
        return {}

    async def get_cost_summary(self, days: int = 30) -> CostSummary:
        cache_key = f"costs_{days}"
        cached = self._cache.get(cache_key)
        if cached:
            return cached

        end_date = date.today()
        start_date = end_date - timedelta(days=days)

        # Query costs by service
        service_query = {
            "type": "ActualCost",
            "timeframe": "Custom",
            "timePeriod": {
                "from": start_date.isoformat(),
                "to": end_date.isoformat(),
            },
            "dataset": {
                "granularity": "None",
                "aggregation": {"totalCost": {"name": "Cost", "function": "Sum"}},
                "grouping": [{"type": "Dimension", "name": "ServiceName"}],
            },
        }
        service_result = await self._query_costs(service_query)

        # Query daily costs
        daily_query = {
            "type": "ActualCost",
            "timeframe": "Custom",
            "timePeriod": {
                "from": start_date.isoformat(),
                "to": end_date.isoformat(),
            },
            "dataset": {
                "granularity": "Daily",
                "aggregation": {"totalCost": {"name": "Cost", "function": "Sum"}},
            },
        }
        daily_result = await self._query_costs(daily_query)

        # Parse service costs
        costs_by_service = []
        total_cost = 0.0
        for row in service_result.get("properties", {}).get("rows", []):
            cost = row[0]
            service_name = row[1]
            currency = row[2] if len(row) > 2 else "USD"
            costs_by_service.append(
                CostByService(service_name=service_name, cost=cost, currency=currency)
            )
            total_cost += cost

        # Parse daily costs
        daily_costs = []
        for row in daily_result.get("properties", {}).get("rows", []):
            cost = row[0]
            date_val = row[1]
            # Azure returns date as YYYYMMDD integer
            if isinstance(date_val, int):
                date_str = str(date_val)
                day_date = date(
                    int(date_str[:4]), int(date_str[4:6]), int(date_str[6:8])
                )
            else:
                day_date = date.fromisoformat(str(date_val)[:10])
            daily_costs.append(DailyCost(date=day_date, cost=cost))

        # Sort by cost descending
        costs_by_service.sort(key=lambda x: x.cost, reverse=True)
        daily_costs.sort(key=lambda x: x.date)

        result = CostSummary(
            subscription_id=self.subscription_id,
            subscription_name="virtual-bpm-prod",
            period_start=start_date,
            period_end=end_date,
            total_cost=total_cost,
            costs_by_service=costs_by_service,
            daily_costs=daily_costs,
        )
        self._cache.set(cache_key, result)
        return result

    async def get_forecast(self) -> CostForecast:
        cache_key = "forecast"
        cached = self._cache.get(cache_key)
        if cached:
            return cached

        end_of_month = date.today().replace(day=28) + timedelta(days=4)
        end_of_month = end_of_month.replace(day=1) - timedelta(days=1)

        query = {
            "type": "ActualCost",
            "timeframe": "MonthToDate",
            "dataset": {
                "granularity": "None",
                "aggregation": {"totalCost": {"name": "Cost", "function": "Sum"}},
            },
        }
        result = await self._query_costs(query)

        rows = result.get("properties", {}).get("rows", [])
        current_cost = rows[0][0] if rows else 0.0

        # Simple linear forecast
        today = date.today()
        days_elapsed = today.day
        days_in_month = end_of_month.day
        estimated_total = (current_cost / days_elapsed) * days_in_month if days_elapsed > 0 else 0

        forecast = CostForecast(
            subscription_id=self.subscription_id,
            forecast_period_end=end_of_month,
            estimated_total=round(estimated_total, 2),
        )
        self._cache.set(cache_key, forecast)
        return forecast
