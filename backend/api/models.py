from pydantic import BaseModel
from datetime import date


class CostByService(BaseModel):
    service_name: str
    cost: float
    currency: str = "USD"


class DailyCost(BaseModel):
    date: date
    cost: float
    currency: str = "USD"


class CostSummary(BaseModel):
    subscription_id: str
    subscription_name: str
    period_start: date
    period_end: date
    total_cost: float
    currency: str = "USD"
    costs_by_service: list[CostByService]
    daily_costs: list[DailyCost]


class CostForecast(BaseModel):
    subscription_id: str
    forecast_period_end: date
    estimated_total: float
    currency: str = "USD"


class HealthCheck(BaseModel):
    status: str
    version: str
