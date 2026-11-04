package {{package}};

import net.fabricmc.api.ModInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class {{mod_class}} implements ModInitializer {
    public static final String MOD_ID = "{{mod_id}}";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

    @Override
    public void onInitialize() {
        LOGGER.info("[DailyPlugin] Mod '{{mod_name}}' has been initialized");
    }
}