/*!
 * \file file_signal_source.cc
 * \brief Brief description of the file here
 * \author Carlos Aviles, 2010. carlos.avilesr(at)googlemail.com
 *         Javier Arribas, 2011 jarribas(at)cttc.es
 *
 * Detailed description of the file here if needed.
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2011  (see AUTHORS file for a list of contributors)
 *
 * GNSS-SDR is a software defined Global Navigation
 *          Satellite Systems receiver
 *
 * This file is part of GNSS-SDR.
 *
 * GNSS-SDR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * at your option) any later version.
 *
 * GNSS-SDR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNSS-SDR. If not, see <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------
 */

#include "file_signal_source.h"

#include "gnss_sdr_valve.h"
#include "configuration_interface.h"

#include <string>
#include <iostream>

#include <glog/log_severity.h>
#include <glog/logging.h>

#include <gnuradio/gr_io_signature.h>

using google::LogMessage;

FileSignalSource::FileSignalSource(ConfigurationInterface* configuration,
        std::string role, unsigned int in_streams, unsigned int out_streams,
        gr_msg_queue_sptr queue) :
    role_(role), in_streams_(in_streams), out_streams_(out_streams), queue_(
            queue)
{

    std::string default_filename = "./signal_samples/signal.dat";
    std::string default_item_type = "short";
    std::string default_dump_filename = "./data/signal_source.dat";

    samples_ = configuration->property(role + ".samples", 0);
    sampling_frequency_ = configuration->property(role
            + ".sampling_frequency", 0);
    filename_ = configuration->property(role + ".filename", default_filename);
    item_type_ = configuration->property(role + ".item_type",
            default_item_type);
    repeat_ = configuration->property(role + ".repeat", false);
    dump_ = configuration->property(role + ".dump", false);
    dump_filename_ = configuration->property(role + ".dump_filename",
            default_dump_filename);
    enable_throttle_control_ = configuration->property(role
            + ".enable_throttle_control", false);

    if (item_type_.compare("gr_complex") == 0)
    {
        item_size_ = sizeof(gr_complex);
    }
    else if (item_type_.compare("float") == 0)
    {
        item_size_ = sizeof(float);
    }
    else if (item_type_.compare("short") == 0)
    {
        item_size_ = sizeof(short);
    }
    else
    {
        LOG_AT_LEVEL(WARNING) << item_type_
                << " unrecognized item type. Using short.";
        item_size_ = sizeof(short);
    }

    file_source_
            = gr_make_file_source(item_size_, filename_.c_str(), repeat_);
    DLOG(INFO) << "file_source(" << file_source_->unique_id() << ")";

    // if samples != 0 then enable a flow valve to stop the process after n samples
    if (samples_ != 0)
    {
        valve_ = gnss_sdr_make_valve(item_size_, samples_, queue_);
        DLOG(INFO) << "valve(" << valve_->unique_id() << ")";
    }

    if (dump_)
    {
        sink_ = gr_make_file_sink(item_size_, dump_filename_.c_str());
        DLOG(INFO) << "file_sink(" << sink_->unique_id() << ")";
    }

    if (enable_throttle_control_)
    {
        throttle_ = gr_make_throttle(item_size_, sampling_frequency_);
    }
    DLOG(INFO) << "File source filename " << filename_;
    DLOG(INFO) << "Samples " << samples_;
    DLOG(INFO) << "Sampling frequency " << sampling_frequency_;
    DLOG(INFO) << "Item type " << item_type_;
    DLOG(INFO) << "Item size " << item_size_;
    DLOG(INFO) << "Repeat " << repeat_;
    DLOG(INFO) << "Dump " << dump_;
    DLOG(INFO) << "Dump filename " << dump_filename_;

}

FileSignalSource::~FileSignalSource()
{
}

void FileSignalSource::connect(gr_top_block_sptr top_block)
{
    if (samples_ != 0)
    {
        if (enable_throttle_control_ == true)
        {
            top_block->connect(file_source_, 0, throttle_, 0);
            DLOG(INFO) << "connected file source to throttle";
            top_block->connect(throttle_, 0, valve_, 0);
            DLOG(INFO) << "connected throttle to valve";
            if (dump_)
            {
                top_block->connect(valve_, 0, sink_, 0);
                DLOG(INFO) << "connected valve to file sink";
            }
        }
        else
        {
            top_block->connect(file_source_, 0, valve_, 0);
            DLOG(INFO) << "connected file source to valve";
            if (dump_)
            {
                top_block->connect(valve_, 0, sink_, 0);
                DLOG(INFO) << "connected valve to file sink";
            }
        }
    }
    else
    {
        if (enable_throttle_control_ == true)
        {
            top_block->connect(file_source_, 0, throttle_, 0);
            DLOG(INFO) << "connected file source to throttle";
            if (dump_)
            {
                top_block->connect(file_source_, 0, sink_, 0);
                DLOG(INFO) << "connected file source to sink";
            }
        }
        else
        {
            if (dump_)
            {
                top_block->connect(file_source_, 0, sink_, 0);
                DLOG(INFO) << "connected file source to sink";
            }
        }
    }
}

void FileSignalSource::disconnect(gr_top_block_sptr top_block)
{
    if (samples_ != 0)
    {
        if (enable_throttle_control_ == true)
        {
            top_block->disconnect(file_source_, 0, throttle_, 0);
            DLOG(INFO) << "disconnected file source to throttle";
            top_block->disconnect(throttle_, 0, valve_, 0);
            DLOG(INFO) << "disconnected throttle to valve";
            if (dump_)
            {
                top_block->disconnect(valve_, 0, sink_, 0);
                DLOG(INFO) << "disconnected valve to file sink";
            }
        }
        else
        {
            top_block->disconnect(file_source_, 0, valve_, 0);
            DLOG(INFO) << "disconnected file source to valve";
            if (dump_)
            {
                top_block->disconnect(valve_, 0, sink_, 0);
                DLOG(INFO) << "disconnected valve to file sink";
            }
        }
    }
    else
    {
        if (enable_throttle_control_ == true)
        {
            top_block->disconnect(file_source_, 0, throttle_, 0);
            DLOG(INFO) << "disconnected file source to throttle";
            if (dump_)
            {
                top_block->disconnect(file_source_, 0, sink_, 0);
                DLOG(INFO) << "disconnected file source to sink";
            }
        }
        else
        {
            if (dump_)
            {
                top_block->disconnect(file_source_, 0, sink_, 0);
                DLOG(INFO) << "disconnected file source to sink";
            }
        }
    }
}

gr_basic_block_sptr FileSignalSource::get_left_block()
{
    LOG_AT_LEVEL(WARNING)
            << "Left block of a signal source should not be retrieved";
    return gr_block_sptr();
}

gr_basic_block_sptr FileSignalSource::get_right_block()
{
    if (samples_ != 0)
    {
        return valve_;
    }
    else
    {
        if (enable_throttle_control_ == true)
        {
            return throttle_;
        }
        else
        {
            return file_source_;
        }
    }
}
