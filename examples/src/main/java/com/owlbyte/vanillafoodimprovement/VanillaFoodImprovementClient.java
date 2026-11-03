package com.owlbyte.vanillafoodimprovement;

import net.fabricmc.api.ClientModInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class VanillaFoodImprovementClient implements ClientModInitializer {
    public static final Logger LOGGER = LoggerFactory.getLogger("vanilla-food-improvement-client");

    @Override
    public void onInitializeClient() {
        LOGGER.info("原版食物改进客户端初始化完成");
    }
}