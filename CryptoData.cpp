#include "CryptoData.h"
#include "ConfigManager.h"
#include "APIManager.h"
#include <ArduinoJson.h>

// ===== STATIC VARIABLES =====
CryptoData* CryptoData::_instance = nullptr;

// ===== CONSTANTS =====
#define MAX_SYMBOL_LENGTH 16
#define MAX_POSITIONS 100
#define POSITION_HISTORY_SIZE 50

// ===== CONSTRUCTOR/DESTRUCTOR =====
CryptoData::CryptoData()
    : _initialized(false),
      _entryPositionCount(0),
      _exitPositionCount(0),
      _lastUpdateTime(0),
      _updateInterval(15000) { // 15 seconds
}

CryptoData::~CryptoData() {
    if (_instance == this) {
        _instance = nullptr;
    }
}

// ===== INITIALIZATION =====
bool CryptoData::begin() {
    Serial.println("Initializing Crypto Data Manager...");
    
    // Clear all data
    clearAllData();
    
    // Load historical data if available
    loadHistoricalData();
    
    _initialized = true;
    Serial.println("Crypto Data Manager initialized");
    
    return true;
}

void CryptoData::update() {
    if (!_initialized) return;
    
    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < _updateInterval) return;
    
    // Update will be triggered externally via fetchData()
    // This method is for internal updates like calculating averages
}

// ===== DATA PARSING =====
bool CryptoData::parsePortfolioData(const String& jsonData, bool isExitMode) {
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
        return false;
    }
    
    if (!doc.containsKey("portfolio")) {
        Serial.println("No 'portfolio' field in JSON");
        return false;
    }
    
    JsonArray portfolio = doc["portfolio"];
    int itemCount = portfolio.size();
    
    // Clear existing data for this mode
    if (isExitMode) {
        _exitPositionCount = 0;
    } else {
        _entryPositionCount = 0;
    }
    
    // Parse positions
    int parsedCount = 0;
    for (JsonObject item : portfolio) {
        if (parsedCount >= MAX_POSITIONS) {
            Serial.println("Warning: Maximum positions reached");
            break;
        }
        
        CryptoPosition* position;
        if (isExitMode) {
            position = &_exitPositions[_exitPositionCount];
        } else {
            position = &_entryPositions[_entryPositionCount];
        }
        
        if (parsePosition(item, *position)) {
            parsedCount++;
            
            if (isExitMode) {
                _exitPositionCount++;
            } else {
                _entryPositionCount++;
            }
        }
    }
    
    // Parse summary
    if (doc.containsKey("summary")) {
        JsonObject summary = doc["summary"];
        parseSummary(summary, isExitMode);
    }
    
    // Calculate derived metrics
    calculateDerivedMetrics(isExitMode);
    
    // Update history
    updatePositionHistory(isExitMode);
    
    _lastUpdateTime = millis();
    
    Serial.print("Parsed ");
    Serial.print(parsedCount);
    Serial.print(" positions for ");
    Serial.println(isExitMode ? "Exit Mode" : "Entry Mode");
    
    return parsedCount > 0;
}

bool CryptoData::parsePosition(JsonObject& item, CryptoPosition& position) {
    // Clear position
    memset(&position, 0, sizeof(CryptoPosition));
    
    // Parse required fields
    const char* symbol = item["symbol"] | "UNKNOWN";
    strncpy(position.symbol, symbol, MAX_SYMBOL_LENGTH - 1);
    position.symbol[MAX_SYMBOL_LENGTH - 1] = '\0';
    
    position.changePercent = item["pnl_percent"] | 0.0;
    position.currentPrice = item["current_price"] | 0.0;
    position.entryPrice = item["entry_price"] | 0.0;
    position.quantity = item["quantity"] | 0.0;
    position.pnlValue = item["pnl"] | 0.0;
    
    // Parse position side
    position.isLong = true; // Default to long
    
    if (item.containsKey("position")) {
        const char* side = item["position"];
        if (strcasecmp(side, "short") == 0) position.isLong = false;
    } else if (item.containsKey("position_side")) {
        const char* side = item["position_side"];
        if (strcasecmp(side, "short") == 0) position.isLong = false;
    } else if (item.containsKey("side")) {
        const char* side = item["side"];
        if (strcasecmp(side, "sell") == 0) position.isLong = false;
    }
    
    // Parse additional fields if available
    if (item.containsKey("leverage")) {
        position.leverage = item["leverage"];
    }
    
    if (item.containsKey("liquidation_price")) {
        position.liquidationPrice = item["liquidation_price"];
    }
    
    if (item.containsKey("margin_type")) {
        const char* marginType = item["margin_type"];
        strncpy(position.marginType, marginType, 11);
        position.marginType[11] = '\0';
    }
    
    // Set alert thresholds from config
    position.alertThreshold = ConfigManager::getInstance().getAlertThreshold();
    position.severeThreshold = ConfigManager::getInstance().getSevereThreshold();
    
    // Initialize alert flags
    position.alerted = false;
    position.severeAlerted = false;
    position.hasAlerted = false;
    position.lastAlertTime = 0;
    position.lastAlertPrice = 0.0;
    position.lastAlertPercent = 0.0;
    
    // For exit mode
    position.exitAlerted = false;
    position.exitAlertLastPrice = position.currentPrice;
    position.exitAlertTime = 0;
    
    return true;
}

void CryptoData::parseSummary(JsonObject& summary, bool isExitMode) {
    PortfolioSummary* portfolioSummary;
    if (isExitMode) {
        portfolioSummary = &_exitSummary;
    } else {
        portfolioSummary = &_entrySummary;
    }
    
    portfolioSummary->totalInvestment = summary["total_investment"] | 0.0;
    portfolioSummary->totalCurrentValue = summary["total_current_value"] | 0.0;
    portfolioSummary->totalPnl = summary["total_pnl"] | 0.0;
    
    // Calculate percentage
    if (portfolioSummary->totalInvestment > 0) {
        portfolioSummary->totalPnlPercent = 
            ((portfolioSummary->totalCurrentValue - portfolioSummary->totalInvestment) / 
             portfolioSummary->totalInvestment) * 100;
    } else {
        portfolioSummary->totalPnlPercent = 0.0;
    }
    
    // Count positions
    portfolioSummary->totalPositions = isExitMode ? _exitPositionCount : _entryPositionCount;
    
    // Count longs/shorts, winners/losers
    portfolioSummary->longPositions = 0;
    portfolioSummary->shortPositions = 0;
    portfolioSummary->winningPositions = 0;
    portfolioSummary->losingPositions = 0;
    
    CryptoPosition* positions;
    int count;
    
    if (isExitMode) {
        positions = _exitPositions;
        count = _exitPositionCount;
    } else {
        positions = _entryPositions;
        count = _entryPositionCount;
    }
    
    for (int i = 0; i < count; i++) {
        if (positions[i].isLong) {
            portfolioSummary->longPositions++;
        } else {
            portfolioSummary->shortPositions++;
        }
        
        if (positions[i].changePercent >= 0) {
            portfolioSummary->winningPositions++;
        } else {
            portfolioSummary->losingPositions++;
        }
    }
    
    // Additional metrics if available
    portfolioSummary->maxDrawdown = summary["max_drawdown"] | 0.0;
    portfolioSummary->sharpeRatio = summary["sharpe_ratio"] | 0.0;
    portfolioSummary->avgPositionSize = summary["avg_position_size"] | 0.0;
    portfolioSummary->riskExposure = summary["risk_exposure"] | 0.0;
}

// ===== DATA FETCHING =====
bool CryptoData::fetchData(bool isExitMode) {
    String portfolioName;
    if (isExitMode) {
        portfolioName = ConfigManager::getInstance().getExitPortfolio();
    } else {
        portfolioName = ConfigManager::getInstance().getEntryPortfolio();
    }
    
    if (portfolioName.isEmpty()) {
        Serial.println("Portfolio name not configured");
        return false;
    }
    
    String response;
    APIManager::APIResponseInfo responseInfo;
    
    bool success = APIManager::getInstance().fetchPortfolioData(
        portfolioName, isExitMode, response, &responseInfo);
    
    if (success) {
        // Parse the response
        bool parseSuccess = parsePortfolioData(response, isExitMode);
        
        if (parseSuccess) {
            // Save successful update
            saveDataSnapshot(isExitMode);
            return true;
        } else {
            Serial.println("Failed to parse portfolio data");
            return false;
        }
    } else {
        Serial.print("Failed to fetch portfolio data: ");
        Serial.println(responseInfo.error);
        return false;
    }
}

bool CryptoData::fetchAllData() {
    bool entrySuccess = false;
    bool exitSuccess = false;
    
    // Fetch entry data
    String entryPortfolio = ConfigManager::getInstance().getEntryPortfolio();
    if (!entryPortfolio.isEmpty()) {
        entrySuccess = fetchData(false);
    }
    
    // Fetch exit data
    String exitPortfolio = ConfigManager::getInstance().getExitPortfolio();
    if (!exitPortfolio.isEmpty()) {
        exitSuccess = fetchData(true);
    }
    
    return entrySuccess || exitSuccess;
}

// ===== DATA ANALYSIS =====
void CryptoData::calculateDerivedMetrics(bool isExitMode) {
    CryptoPosition* positions;
    int count;
    PortfolioSummary* summary;
    
    if (isExitMode) {
        positions = _exitPositions;
        count = _exitPositionCount;
        summary = &_exitSummary;
    } else {
        positions = _entryPositions;
        count = _entryPositionCount;
        summary = &_entrySummary;
    }
    
    if (count == 0) return;
    
    // Calculate additional metrics
    float totalExposure = 0;
    float maxLoss = 0;
    float maxGain = 0;
    
    for (int i = 0; i < count; i++) {
        totalExposure += positions[i].currentPrice * positions[i].quantity;
        
        if (positions[i].changePercent < maxLoss) {
            maxLoss = positions[i].changePercent;
        }
        
        if (positions[i].changePercent > maxGain) {
            maxGain = positions[i].changePercent;
        }
    }
    
    summary->maxDrawdown = maxLoss;
    summary->riskExposure = totalExposure;
    
    // Calculate average position size
    if (count > 0) {
        summary->avgPositionSize = totalExposure / count;
    }
}

void CryptoData::sortPositionsByPNL(bool isExitMode, bool ascending) {
    CryptoPosition* positions;
    int count;
    
    if (isExitMode) {
        positions = _exitPositions;
        count = _exitPositionCount;
    } else {
        positions = _entryPositions;
        count = _entryPositionCount;
    }
    
    // Simple bubble sort
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            bool shouldSwap = false;
            
            if (ascending) {
                shouldSwap = positions[j].changePercent > positions[j + 1].changePercent;
            } else {
                shouldSwap = positions[j].changePercent < positions[j + 1].changePercent;
            }
            
            if (shouldSwap) {
                CryptoPosition temp = positions[j];
                positions[j] = positions[j + 1];
                positions[j + 1] = temp;
            }
        }
    }
}

// ===== POSITION HISTORY =====
void CryptoData::updatePositionHistory(bool isExitMode) {
    CryptoPosition* positions;
    int count;
    std::vector<PositionHistory>* history;
    
    if (isExitMode) {
        positions = _exitPositions;
        count = _exitPositionCount;
        history = &_exitPositionHistory;
    } else {
        positions = _entryPositions;
        count = _entryPositionCount;
        history = &_entryPositionHistory;
    }
    
    unsigned long currentTime = millis();
    
    // Update history for each position
    for (int i = 0; i < count; i++) {
        // Find existing history for this symbol
        bool found = false;
        for (auto& hist : *history) {
            if (strcmp(hist.symbol, positions[i].symbol) == 0) {
                // Update existing history
                hist.lastPrice = positions[i].currentPrice;
                hist.lastUpdate = currentTime;
                hist.changePercent = positions[i].changePercent;
                
                // Add to price history
                if (hist.priceHistory.size() >= POSITION_HISTORY_SIZE) {
                    hist.priceHistory.erase(hist.priceHistory.begin());
                }
                hist.priceHistory.push_back(positions[i].currentPrice);
                
                found = true;
                break;
            }
        }
        
        // Create new history if not found
        if (!found) {
            PositionHistory newHist;
            strncpy(newHist.symbol, positions[i].symbol, MAX_SYMBOL_LENGTH - 1);
            newHist.symbol[MAX_SYMBOL_LENGTH - 1] = '\0';
            newHist.lastPrice = positions[i].currentPrice;
            newHist.lastUpdate = currentTime;
            newHist.changePercent = positions[i].changePercent;
            newHist.priceHistory.push_back(positions[i].currentPrice);
            
            history->push_back(newHist);
            
            // Limit history size
            if (history->size() > 20) {
                history->erase(history->begin());
            }
        }
    }
}

// ===== DATA PERSISTENCE =====
void CryptoData::saveDataSnapshot(bool isExitMode) {
    // Save to preferences
    String prefix = isExitMode ? "exit_data" : "entry_data";
    
    _prefs.begin(prefix.c_str(), false);
    
    // Save timestamp
    _prefs.putULong("last_update", millis());
    
    // Save summary
    PortfolioSummary* summary;
    if (isExitMode) {
        summary = &_exitSummary;
    } else {
        summary = &_entrySummary;
    }
    
    _prefs.putFloat("total_investment", summary->totalInvestment);
    _prefs.putFloat("total_current_value", summary->totalCurrentValue);
    _prefs.putFloat("total_pnl", summary->totalPnl);
    _prefs.putFloat("total_pnl_percent", summary->totalPnlPercent);
    _prefs.putUInt("total_positions", summary->totalPositions);
    
    _prefs.end();
    
    // Save detailed data to SPIFFS if needed
    saveDetailedDataToFile(isExitMode);
}

void CryptoData::loadHistoricalData() {
    // Load entry data
    _prefs.begin("entry_data", true);
    _entrySummary.totalInvestment = _prefs.getFloat("total_investment", 0);
    _entrySummary.totalCurrentValue = _prefs.getFloat("total_current_value", 0);
    _entrySummary.totalPnl = _prefs.getFloat("total_pnl", 0);
    _entrySummary.totalPnlPercent = _prefs.getFloat("total_pnl_percent", 0);
    _prefs.end();
    
    // Load exit data
    _prefs.begin("exit_data", true);
    _exitSummary.totalInvestment = _prefs.getFloat("total_investment", 0);
    _exitSummary.totalCurrentValue = _prefs.getFloat("total_current_value", 0);
    _exitSummary.totalPnl = _prefs.getFloat("total_pnl", 0);
    _exitSummary.totalPnlPercent = _prefs.getFloat("total_pnl_percent", 0);
    _prefs.end();
}

void CryptoData::saveDetailedDataToFile(bool isExitMode) {
    // This would save detailed data to SPIFFS
    // Implement based on your storage needs
}

// ===== DATA QUERY METHODS =====
CryptoPosition* CryptoData::getPosition(const char* symbol, bool isExitMode) {
    CryptoPosition* positions;
    int count;
    
    if (isExitMode) {
        positions = _exitPositions;
        count = _exitPositionCount;
    } else {
        positions = _entryPositions;
        count = _entryPositionCount;
    }
    
    for (int i = 0; i < count; i++) {
        if (strcmp(positions[i].symbol, symbol) == 0) {
            return &positions[i];
        }
    }
    
    return nullptr;
}

CryptoPosition* CryptoData::getWorstPosition(bool isExitMode) {
    CryptoPosition* positions;
    int count;
    
    if (isExitMode) {
        positions = _exitPositions;
        count = _exitPositionCount;
    } else {
        positions = _entryPositions;
        count = _entryPositionCount;
    }
    
    if (count == 0) return nullptr;
    
    CryptoPosition* worst = &positions[0];
    for (int i = 1; i < count; i++) {
        if (positions[i].changePercent < worst->changePercent) {
            worst = &positions[i];
        }
    }
    
    return worst;
}

CryptoPosition* CryptoData::getBestPosition(bool isExitMode) {
    CryptoPosition* positions;
    int count;
    
    if (isExitMode) {
        positions = _exitPositions;
        count = _exitPositionCount;
    } else {
        positions = _entryPositions;
        count = _entryPositionCount;
    }
    
    if (count == 0) return nullptr;
    
    CryptoPosition* best = &positions[0];
    for (int i = 1; i < count; i++) {
        if (positions[i].changePercent > best->changePercent) {
            best = &positions[i];
        }
    }
    
    return best;
}

// ===== WEB INTERFACE =====
String CryptoData::getDataJSON(bool isExitMode) {
    DynamicJsonDocument doc(8192);
    
    PortfolioSummary* summary;
    CryptoPosition* positions;
    int count;
    
    if (isExitMode) {
        summary = &_exitSummary;
        positions = _exitPositions;
        count = _exitPositionCount;
        doc["mode"] = "exit";
    } else {
        summary = &_entrySummary;
        positions = _entryPositions;
        count = _entryPositionCount;
        doc["mode"] = "entry";
    }
    
    // Summary
    JsonObject summaryObj = doc.createNestedObject("summary");
    summaryObj["totalInvestment"] = summary->totalInvestment;
    summaryObj["totalCurrentValue"] = summary->totalCurrentValue;
    summaryObj["totalPnl"] = summary->totalPnl;
    summaryObj["totalPnlPercent"] = summary->totalPnlPercent;
    summaryObj["totalPositions"] = summary->totalPositions;
    summaryObj["longPositions"] = summary->longPositions;
    summaryObj["shortPositions"] = summary->shortPositions;
    summaryObj["winningPositions"] = summary->winningPositions;
    summaryObj["losingPositions"] = summary->losingPositions;
    summaryObj["maxDrawdown"] = summary->maxDrawdown;
    summaryObj["sharpeRatio"] = summary->sharpeRatio;
    
    // Positions
    JsonArray positionsArray = doc.createNestedArray("positions");
    
    for (int i = 0; i < count; i++) {
        JsonObject posObj = positionsArray.createNestedObject();
        posObj["symbol"] = positions[i].symbol;
        posObj["changePercent"] = positions[i].changePercent;
        posObj["pnlValue"] = positions[i].pnlValue;
        posObj["quantity"] = positions[i].quantity;
        posObj["entryPrice"] = positions[i].entryPrice;
        posObj["currentPrice"] = positions[i].currentPrice;
        posObj["isLong"] = positions[i].isLong;
        posObj["alerted"] = positions[i].alerted;
        posObj["severeAlerted"] = positions[i].severeAlerted;
        posObj["lastAlertTime"] = positions[i].lastAlertTime;
        
        if (isExitMode) {
            posObj["exitAlerted"] = positions[i].exitAlerted;
            posObj["exitAlertTime"] = positions[i].exitAlertTime;
        }
    }
    
    // Metadata
    doc["lastUpdate"] = _lastUpdateTime;
    doc["positionCount"] = count;
    
    String json;
    serializeJson(doc, json);
    return json;
}

// ===== UTILITY FUNCTIONS =====
void CryptoData::clearAllData() {
    memset(_entryPositions, 0, sizeof(_entryPositions));
    memset(_exitPositions, 0, sizeof(_exitPositions));
    
    _entryPositionCount = 0;
    _exitPositionCount = 0;
    
    memset(&_entrySummary, 0, sizeof(PortfolioSummary));
    memset(&_exitSummary, 0, sizeof(PortfolioSummary));
    
    _entryPositionHistory.clear();
    _exitPositionHistory.clear();
    
    Serial.println("All crypto data cleared");
}

void CryptoData::printSummary(bool isExitMode) {
    PortfolioSummary* summary;
    int count;
    
    if (isExitMode) {
        summary = &_exitSummary;
        count = _exitPositionCount;
        Serial.println("\n=== Exit Mode Summary ===");
    } else {
        summary = &_entrySummary;
        count = _entryPositionCount;
        Serial.println("\n=== Entry Mode Summary ===");
    }
    
    Serial.print("Total Positions: ");
    Serial.println(count);
    Serial.print("Total Investment: $");
    Serial.println(summary->totalInvestment, 2);
    Serial.print("Total Current Value: $");
    Serial.println(summary->totalCurrentValue, 2);
    Serial.print("Total P/L: $");
    Serial.print(summary->totalPnl, 2);
    Serial.print(" (");
    Serial.print(summary->totalPnlPercent, 2);
    Serial.println("%)");
    Serial.print("Long Positions: ");
    Serial.println(summary->longPositions);
    Serial.print("Short Positions: ");
    Serial.println(summary->shortPositions);
    Serial.print("Winning Positions: ");
    Serial.println(summary->winningPositions);
    Serial.print("Losing Positions: ");
    Serial.println(summary->losingPositions);
    Serial.print("Max Drawdown: ");
    Serial.print(summary->maxDrawdown, 2);
    Serial.println("%");
    Serial.println("=======================\n");
}

// ===== GETTERS =====
const CryptoPosition* CryptoData::getPositions(bool isExitMode) const {
    return isExitMode ? _exitPositions : _entryPositions;
}

int CryptoData::getPositionCount(bool isExitMode) const {
    return isExitMode ? _exitPositionCount : _entryPositionCount;
}

const PortfolioSummary& CryptoData::getSummary(bool isExitMode) const {
    return isExitMode ? _exitSummary : _entrySummary;
}

unsigned long CryptoData::getLastUpdateTime() const {
    return _lastUpdateTime;
}

bool CryptoData::hasData(bool isExitMode) const {
    return isExitMode ? (_exitPositionCount > 0) : (_entryPositionCount > 0);
}

bool CryptoData::isInitialized() const {
    return _initialized;
}

// ===== STATIC ACCESS =====
CryptoData& CryptoData::getInstance() {
    if (!_instance) {
        _instance = new CryptoData();
    }
    return *_instance;
}