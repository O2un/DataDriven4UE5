export module ITimeHelper;
import <chrono>;

export namespace CommonHelper
{
    inline std::string GetCurrentYear() 
    {
        auto now = std::chrono::system_clock::now();
        auto current_day = std::chrono::floor<std::chrono::days>(now);
        std::chrono::year_month_day ymd{ current_day };
        return std::to_string(static_cast<int>(ymd.year()));
    }
}