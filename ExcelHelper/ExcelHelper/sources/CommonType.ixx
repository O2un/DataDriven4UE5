export module ICommonType;

import <unordered_map>;
import <string>;

export namespace EXCEL
{
	using SheetIndex = std::unordered_map<std::string, std::string>;
}