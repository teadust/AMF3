#ifndef CPPUTILS_H_
#define CPPUTILS_H_

#include <sstream>
    template<typename T>
    static std::string NumberToString(const T number)
    {
        std::stringstream stream;
        stream << number;
        
        return stream.str();
    }
    
    
    template<typename T>
    static T StringToNumber(const std::string& numberStr)
    {
        T retNum;
        
        std::stringstream stream(numberStr);
        stream >> retNum;
        
        return retNum;
    }

	static void safeReplaceString(std::string& resStr,int position,int size,std::string repStr){
		if(position==-1){
			return;
		}
		resStr.replace(position,size,repStr);
	}

static std::string fixUrl(std::string& urlStr){
    std::string source = urlStr;
    
    if (urlStr.find_last_of("/") > urlStr.length())
        return source;
    
    std::string temp = urlStr.substr(urlStr.find_last_of("/"));
    urlStr = urlStr.substr(0, urlStr.find_last_of("/"));
    
    if (urlStr.find_last_of("/") > urlStr.length())
        return source;
    
    urlStr = urlStr.substr(0, urlStr.find_last_of("/"));
    
    return urlStr + temp;
}
	
#endif
