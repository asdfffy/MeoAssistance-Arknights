﻿#include "CreditShoppingTask.h"

#include <vector>
#include <algorithm>

#include <opencv2/opencv.hpp>

#include "Controller.h"
#include "CreditShopImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"

bool asst::CreditShoppingTask::run()
{
    const cv::Mat& image = ctrler.get_image();

    CreditShopImageAnalyzer shop_analyzer(image);
    if (!shop_analyzer.analyze()) {
        return false;
    }
    const auto& shopping_list = shop_analyzer.get_result();

    for (const Rect& commodity : shopping_list) {
        if (need_exit()) {
            return false;
        }
        ctrler.click(commodity);

        static int pre_delay = 1000;
        static int rare_delay = 1000;

        sleep(pre_delay);

        // “购买商品”按钮
        static Rect buy_it_rect;
        if (buy_it_rect.empty()) {
            const auto buy_it_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
                resource.task().task_ptr("CreditShop-BuyIt"));

            const cv::Mat& buy_image = ctrler.get_image();
            MatchImageAnalyzer buy_it_analyzer(
                buy_image,
                buy_it_task_ptr->roi,
                buy_it_task_ptr->templ_name,
                buy_it_task_ptr->templ_threshold);
            if (!buy_it_analyzer.analyze()) {
                // 没识别到“购买商品”按钮，不应该出现这种情况，TODO 报错
                return false;
            }
            buy_it_rect = buy_it_analyzer.get_result().rect;
            pre_delay = buy_it_task_ptr->pre_delay;
            rare_delay = buy_it_task_ptr->rear_delay;
        }

        if (need_exit()) {
            return false;
        }
        ctrler.click(buy_it_rect);
        sleep(rare_delay);

        // 识别是否信用不足无法购买
        const cv::Mat& prompt_image = ctrler.get_image();

        const static auto no_money_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
            resource.task().task_ptr("CreditShop-NoMoney"));
        OcrImageAnalyzer prompt_analyzer(prompt_image, no_money_task_ptr->roi);
        prompt_analyzer.set_required(no_money_task_ptr->text);
        if (prompt_analyzer.analyze()) {
            click_return_button();
            return true;
        }

        // 这里随便点一下都行，把购买完弹出来物品的界面取消掉
        ctrler.click(buy_it_rect);
        sleep(rare_delay);
    }

    return true;
}