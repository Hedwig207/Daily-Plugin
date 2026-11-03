package com.owlbyte.vanillafoodimprovement.mixin;

import net.minecraft.entity.effect.StatusEffectInstance;
import net.minecraft.entity.effect.StatusEffects;
import net.minecraft.item.FoodComponent;
import net.minecraft.item.Item;
import net.minecraft.item.Items;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/**
 * 让闪烁的西瓜片可以食用，并提供 3 秒再生效果。
 *
 * 通过编译器覆盖 Item 的 isFood / getFoodComponent 逻辑实现，
 * 不替换 Item 实例本身，因此不会破坏注册表（不会触发 intrusive holder 崩溃）。
 */
@Mixin(Item.class)
public abstract class GlisteringMelonSliceMixin {

    private static final FoodComponent GLISTERING_MELON_FOOD = new FoodComponent.Builder()
            .hunger(6)
            .saturationModifier(0.7f)
            .statusEffect(new StatusEffectInstance(StatusEffects.REGENERATION, 60, 0), 1.0f)
            .alwaysEdible()
            .build();

    @Inject(method = "isFood", at = @At("HEAD"), cancellable = true)
    private void vfi_makeGlisteringMelonFood(CallbackInfoReturnable<Boolean> cir) {
        if (Items.GLISTERING_MELON_SLICE == (Item) (Object) this) {
            cir.setReturnValue(true);
        }
    }

    @Inject(method = "getFoodComponent", at = @At("HEAD"), cancellable = true)
    private void vfi_glisteringMelonFoodComponent(CallbackInfoReturnable<FoodComponent> cir) {
        if (Items.GLISTERING_MELON_SLICE == (Item) (Object) this) {
            cir.setReturnValue(GLISTERING_MELON_FOOD);
        }
    }
}