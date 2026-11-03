package {{package}};

import net.minecraftforge.fml.common.Mod;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Mod("{{mod_id}}")
public class {{mod_class}} {
    public static final String MOD_ID = "{{mod_id}}";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

    public {{mod_class}}() {
        LOGGER.info("[DailyPlugin] Mod '{{mod_name}}' has been loaded");
    }
}