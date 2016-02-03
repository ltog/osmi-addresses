#ifndef COMPRESSEDWAY_HPP_
#define COMPRESSEDWAY_HPP_

class CompressedWay {

public:
	CompressedWay(std::unique_ptr<OGRLineString> way) {
		big_coord_pair sum;
		num_points = way.get()->getNumPoints();
		first.x = sum.x = way.get()->getX(0);
		first.y = sum.y = way.get()->getY(0);

		deltas = new small_coord_pair[std::max(0,std::max(0,num_points-2))];
		for (int i=1; i<=num_points-2; i++) { // i is index of way, NOT intermediates!
			deltas[i-1].x = way.get()->getX(i) - sum.x;
			deltas[i-1].y = way.get()->getY(i) - sum.y;
			sum.x += deltas[i-1].x;
			sum.y += deltas[i-1].y;
		}

		last.x = way.get()->getX(num_points-1);
		last.y = way.get()->getY(num_points-1);
	}

	~CompressedWay() {
		delete[] deltas;
	}

	/* returns a unique_ptr to a newly created (cloned) OGRLineString */
	std::unique_ptr<OGRLineString> uncompress() {
		std::unique_ptr<OGRLineString> linestring(new OGRLineString);
		linestring.get()->addPoint(first.x, first.y);
		big_coord_pair prev;
		prev.x = first.x;
		prev.y = first.y;
		for (int i=0; i<=num_points-3; i++) {
			linestring.get()->addPoint(prev.x + deltas[i].x, prev.y + deltas[i].y);
			prev.x += deltas[i].x;
			prev.y += deltas[i].y;
		}
		linestring.get()->addPoint(last.x, last.y);

		return linestring;
	}

	#pragma pack(push, 1)
		struct big_coord_pair {
			double x;
			double y;
		};

		struct small_coord_pair {
			float x;
			float y;
		};
	#pragma pack(pop)

private:
	#pragma pack(push, 1)
		big_coord_pair first, last;
		small_coord_pair* deltas;
		unsigned short num_points;
	#pragma pack(pop)

};

#endif /* COMPRESSEDWAY_HPP_ */
