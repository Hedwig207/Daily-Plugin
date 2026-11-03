package com.owlbyte.vanillafoodimprovement;

import net.fabricmc.api.ModInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class VanillaFoodImprovement implements ModInitializer {
    public static final String MOD_ID = "vanilla-food-improvement";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

    @Override
    public void onInitialize() {
        LOGGER.info("正在初始化原版食物改进模组...");
        LOGGER.info("版本: 0.1.0-alpha_snapshot-2");

        LOGGER.info("✓ 已添加: 附魔金苹果合成配方");
        LOGGER.info("✓ 已添加: 闪烁的西瓜片可食用 (3秒再生效果) [通过Mixin]");
        LOGGER.info("模组加载完成!");
    }
}