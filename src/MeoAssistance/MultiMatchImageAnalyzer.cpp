﻿#include "MultiMatchImageAnalyzer.h"

#include "Logger.hpp"
#include "Resource.h"

asst::MultiMatchImageAnalyzer::MultiMatchImageAnalyzer(const cv::Mat& image, const Rect& roi, std::string templ_name, double templ_thres)
    : AbstractImageAnalyzer(image, roi),
    m_templ_name(templ_name),
    m_templ_thres(templ_thres)
{
    ;
}

bool asst::MultiMatchImageAnalyzer::analyze()
{
    log.trace("MultiMatchImageAnalyzer::analyze | ", m_templ_name);
    m_result.clear();

    const cv::Mat& templ = resource.templ().get_templ(m_templ_name);
    if (templ.empty()) {
        log.error("templ is empty!");
        return false;
    }

    return multi_match_templ(templ);
}

void asst::MultiMatchImageAnalyzer::sort_result()
{
    // 按位置排个序
    std::sort(m_result.begin(), m_result.end(),
        [](const MatchRect& lhs, const MatchRect& rhs) -> bool {
            if (std::abs(lhs.rect.y - rhs.rect.y) < 5) {	// y差距较小则理解为是同一排的，按x排序
                return lhs.rect.x < rhs.rect.x;
            }
            else {
                return lhs.rect.y < rhs.rect.y;
            }
        }
    );
}

bool asst::MultiMatchImageAnalyzer::multi_match_templ(const cv::Mat& templ)
{
    cv::Mat matched;
    cv::Mat image_roi = m_image(utils::make_rect<cv::Rect>(m_roi));
    if (m_mask_range.first == m_mask_range.second) {
        cv::matchTemplate(image_roi, templ, matched, cv::TM_CCOEFF_NORMED);
    }
    else {
        cv::Mat mask;
        cv::cvtColor(templ, mask, cv::COLOR_BGR2GRAY);
        cv::threshold(mask, mask, m_mask_range.first, m_mask_range.second, cv::THRESH_BINARY);
        cv::matchTemplate(image_roi, templ, matched, cv::TM_CCOEFF_NORMED, mask);
    }

    int mini_distance = (std::min)(templ.cols, templ.rows) * 0.5;
    for (int i = 0; i != matched.rows; ++i) {
        for (int j = 0; j != matched.cols; ++j) {
            auto value = matched.at<float>(i, j);
            if (value >= m_templ_thres) {
                Rect rect(j + m_roi.x, i + m_roi.y, templ.cols, templ.rows);
                bool need_push = true;
                // 如果有两个点离得太近，只取里面得分高的那个
                // 一般相邻的都是刚刚push进去的，这里倒序快一点
                for (auto iter = m_result.rbegin(); iter != m_result.rend(); ++iter) {
                    if (std::abs(j + m_roi.x - iter->rect.x) < mini_distance
                        && std::abs(i + m_roi.y - iter->rect.y) < mini_distance)
                    {
                        if (iter->score < value) {
                            iter->rect = rect;
                            iter->score = value;
                        }	// else 这个点就放弃了
                        need_push = false;
                        break;
                    }
                }
                if (need_push) {
                    m_result.emplace_back(MatchRect{ AlgorithmType::MatchTemplate, value, rect });
                }
            }
        }
    }
    if (!m_result.empty()) {
        return true;
    }

    return false;
}