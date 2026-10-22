#include "Catalog.h"

const json::Value& shopCatalog() {
    static const json::Value catalog = [] {
      auto result = json::Value::parse(R"([
      {"id":1,"name":"Bread","description":"A filling snack at a friendly price.","category":"food","categoryLabel":"Food","filter":"food","price":5,"hungerRestore":30,"happinessRestore":0,"image":"assets/items/banh_mi.png"},
      {"id":2,"name":"Apple","description":"A crisp and refreshing fruit.","category":"food","categoryLabel":"Food","filter":"food","price":8,"hungerRestore":20,"happinessRestore":5,"image":"assets/items/tao_do.png"},
      {"id":3,"name":"Golden Apple","description":"A rare apple with a generous boost.","category":"food","categoryLabel":"Food","filter":"food","price":25,"hungerRestore":40,"happinessRestore":15,"image":"assets/items/tao_vang.png"},
      {"id":4,"name":"Cream Cake","description":"A soft, sweet treat that lifts the mood.","category":"food","categoryLabel":"Food","filter":"food","price":6,"hungerRestore":10,"happinessRestore":20,"image":"assets/items/banh_kem.png"},
      {"id":5,"name":"Honey","description":"Naturally sweet and nourishing.","category":"food","categoryLabel":"Food","filter":"food","price":3,"hungerRestore":5,"happinessRestore":25,"image":"assets/items/mat_ong.png"},
      {"id":6,"name":"Wool Hat","description":"A warm knitted hat for your pet.","category":"decoration","categoryLabel":"Decoration \u00b7 Headwear","filter":"headwear","price":15,"image":"assets/items/mu_len.png"},
      {"id":7,"name":"Crown","description":"A royal crown for your pet.","category":"decoration","categoryLabel":"Decoration \u00b7 Headwear","filter":"headwear","price":50,"image":"assets/items/vuong_mien.png"},
      {"id":8,"name":"Beach Escape","description":"A sunny shore with clear blue water.","category":"decoration","categoryLabel":"Decoration \u00b7 Background","filter":"background","price":20,"image":"assets/items/nen_bai_bien.png"},
      {"id":9,"name":"Starry Sky","description":"A calm night sky filled with stars.","category":"decoration","categoryLabel":"Decoration \u00b7 Background","filter":"background","price":30,"image":"assets/items/nen_bau_troi.png"},
      {"id":12,"name":"Forest Hideaway","description":"A peaceful scene among green trees.","category":"decoration","categoryLabel":"Decoration \u00b7 Background","filter":"background","price":25,"image":"assets/items/nen_khu_rung.png"},
      {"id":13,"name":"City Lights","description":"A lively city glowing after dark.","category":"decoration","categoryLabel":"Decoration \u00b7 Background","filter":"background","price":35,"image":"assets/items/nen_thanh_pho.png"},
      {"id":10,"name":"Wooden Floor","description":"A warm wooden floor for a cozy room.","category":"decoration","categoryLabel":"Decoration \u00b7 Floor","filter":"floor","price":18,"image":"assets/items/san_go.png"},
      {"id":11,"name":"Flower Rug","description":"A soft rug with a cheerful floral pattern.","category":"decoration","categoryLabel":"Decoration \u00b7 Floor","filter":"floor","price":22,"image":"assets/items/tham_hoa.png"}
      ])");
      const auto icons = json::Value::parse(R"(["\ud83e\udd56","\ud83c\udf4e","\ud83c\udf4f","\ud83c\udf70","\ud83c\udf6f","\ud83e\udde2","\ud83d\udc51","\ud83c\udfd6\ufe0f","\ud83c\udf0c","\ud83c\udf33","\ud83c\udf03","\ud83e\udeb5","\ud83c\udf3c"])");
      for (size_t index = 0; index < result.size(); ++index) result[index]["icon"] = icons[index];
      return result;
    }();
    return catalog;
}

const json::Value* shopItem(int id) {
    for (const auto& item : shopCatalog()) if (item["id"] == id) return &item;
    return nullptr;
}