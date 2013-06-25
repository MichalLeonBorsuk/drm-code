/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):  Julian Cable, Ollie Haffenden, Andrew Murphy
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "JMLSCE.h"
#include "JML.h"
#include "timestamp.h"

using namespace JML;

static void createJML(JMLObjectCollection& carousel, const string& sometext)
{
    carousel.add(JMLObject(1, "Business", "it's all going pear shaped"));
    carousel.add(JMLObject(2, "Sports", "Rain stopped play"));
    carousel.add(JMLObject(3, "Entertainment", sometext));
    carousel.add(JMLObject(4, "Science", "CERN makes black hole"));
    vector<Link> link;

    vector<ListBlock> lb(3);
    lb[0].col.resize(2);
    lb[0].col[0]=Text("Team");
    lb[0].col[1]=Text("Score");
    lb[1].col.resize(2);
    lb[1].col[0]=Text("England");
    lb[1].col[1]=Text("1");
    lb[2].col.resize(2);
    lb[2].col[0]=Text("Germany");
    lb[2].col[1]=Text("1");
    carousel.add(JMLObject(5, "Scores", lb));
    link.push_back(Link(carousel[1]));
    link.push_back(Link(carousel[2]));
    link.push_back(Link(carousel[3]));
    link.push_back(Link(carousel[4]));
    link.push_back(Link(carousel[5]));

    carousel.add(JMLObject(0, "News", link));
}


void JMLSCE::ReConfigure(const ServiceComponent& config)
{
  PacketSCE::ReConfigure(config);
  // work out the DGE overhead
  dge.Configure(true, false, false);
  packet_encoder.ReConfigure(config);
  JMLObjectCollection carousel;
  createJML(carousel, "Avengers movie in works");
  data_unit.resize(carousel.size());
  for(size_t i=0; i<carousel.size(); i++)
  {
    carousel[i].encode(data_unit[i]);
  }
}

void JMLSCE::NextFrame(bytevector &out, size_t max, double stoptime)
{
  if(packet_queue.empty())
    fill(stoptime);
  if(packet_queue.empty())
  {
    //cout << "JMLSCE: queue empty" << endl;
    return;
  }
  else
  {
    //cout << "JMLSCE: queue OK" << endl;
  }
  if(packet_queue.front().size()<=max) {
    out.put(packet_queue.front());
    packet_queue.pop();
  } else {
    cerr << "JMLSCE: packet queue size mismatch with packet mux" << endl; cerr.flush();
  }
}

void JMLSCE::fill(double stoptime)
{
  timespec t;
  clock_getrealtime(&t);
  double now_ms = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
  while(packet_queue.size() < max_queue_depth && now_ms < stoptime)
  {
    // put something in the packet queue
    for(size_t i=0; i<data_unit.size(); i++)
    {
        crcbytevector out;
        dge.putDataGroup(0, out, data_unit[i], i);
        packet_encoder.makeDataUnit(packet_queue, out);
    }
    clock_getrealtime(&t);
    now_ms = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
  }
}
