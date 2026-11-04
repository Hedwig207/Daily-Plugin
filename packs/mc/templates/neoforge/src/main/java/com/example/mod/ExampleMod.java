package {{package}};

import net.neoforged.bus.api.IEventBus;
import net.neoforged.fml.common.Mod;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Mod("{{mod_id}}")
public class {{mod_class}} {
    public static final String MOD_ID = "{{mod_id}}";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

    public {{mod_class}}(IEventBus bus) {
        LOGGER.info("[DailyPlugin] Mod '{{mod_name}}' has been initialized");
    }
}