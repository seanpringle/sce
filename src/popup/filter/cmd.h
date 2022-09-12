#pragma once

struct FilterPopupCmd : FilterPopup {
	std::string prefix;
	std::map<std::string,std::vector<std::string>> history;
	std::string key();
	void exec(std::string suffix);

	FilterPopupCmd();
	void init();
	void chosen(int option);
	bool enterable() override { return true; };
	void entered() override;
	std::string hint() override;
};
