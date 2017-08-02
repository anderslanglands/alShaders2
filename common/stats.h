#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <ai.h>


// stats.h
struct Range
{
	Range(const std::string& nm, bool rod=false)
	: name(nm), min(AI_INFINITE), max(-AI_INFINITE), total(0.0), n(0.0), reportOnDestruction(rod)
	{}

	~Range()
	{
		if (reportOnDestruction) report(std::cerr);
	}

	void addSample(double x)
	{
		min = std::min(x, min);
		max = std::max(x, max);
		total += x;
		n++;
	}

	void report(std::ostream& os)
	{
		os << "[" << name << "] " << "min: " << min << " max: " << max << " avg: " << total/n << std::endl;
	}

	std::string name;

	double min;
	double max;
	double total;
	double n;
	bool reportOnDestruction;
};

struct Histogram
{
	Histogram(const std::string& nm, double n, double x, int b, bool nrm, bool rod=false)
	: name(nm), min(n), max(x), total(0.0), bins(NULL), nbins(b), less(0.0), more(0.0), normalize(nrm), reportOnDestruction(rod)
	{
		bins = new double[nbins];
		memset(bins, 0, sizeof(double)*nbins);
	}

	~Histogram()
	{
		if (reportOnDestruction) report(std::cerr);
	}

	void addSample(double x)
	{
		if (x < min) less++;
		else if (x >= max) more++;
		else
		{
			int b = (int)((x-min)/(max-min) * nbins);
			bins[b]++;
		}

		total++;
	}

	void report(std::ostream& os)
	{
		if (normalize)
		{
			less /= total;
			more /= total;
			for (int i = 0; i < nbins; ++i)
			{
				bins[i] /= total;
			}
		}

		os << "[" << name << "] ";
		os << std::setw(10) << less << " ";
		for (int i = 0; i < nbins; ++i)
		{
			os << std::setw(10) << bins[i] << " ";
		}
		os << std::setw(10) << more << " ";
		os << std::endl;
	}

	std::string name;
	double min;
	double max;
	double total;
	double n;
	double* bins;
	int nbins;
	double less;
	double more;
	bool normalize;
	bool reportOnDestruction;
};