# Marisa Patel, 400393635, patem156

import numpy as np # Importing numpy
import open3d as o3d # Importing open3d

if __name__ == "__main__":
    scans = int(input("Enter Number of Scans:"))
    spins = 16
    # Here we are reading the test data from the file that has been created        
    print("Read in the prism point cloud data (pcd)")
    pcd = o3d.io.read_point_cloud("2dx3points.xyz", format="xyz")

    # Here we will see what our point cloud data visually looks like numerically       
    print("The PCD array:")
    print(np.asarray(pcd.points))

    # Here we will see what our point cloud data looks like as a 3D graph in open3D      
    print("Lets visualize the PCD: (spawns seperate interactive window)")
    o3d.visualization.draw_geometries([pcd])

    # Here each vertex will be given a unique number
    yz_slice_vertex = []
    for x in range(0,scans*spins):
        yz_slice_vertex.append([x])

    # Now we will define coordinates in order to connect the lines in each yz plane data        
    lines = []  
    for x in range(0,scans*spins,spins):
        for i in range(spins):
            if i==spins-1:
                lines.append([yz_slice_vertex[x+i], yz_slice_vertex[x]])
            else:
                lines.append([yz_slice_vertex[x+i], yz_slice_vertex[x+i+1]])
            
    # Here we will define coordinates in order to connect lines between current and the upcoming yz plane data       
    for x in range(0,scans*spins-spins-1,spins):
        for i in range(spins):
            lines.append([yz_slice_vertex[x+i], yz_slice_vertex[x+i+spins]])

    # Here, the line will map the lines using the 3D coordinate vertices
    line_set = o3d.geometry.LineSet(points=o3d.utility.Vector3dVector(np.asarray(pcd.points)),lines=o3d.utility.Vector2iVector(lines))

    # Now we will see the final 3D graph of the data collected       
    o3d.visualization.draw_geometries([line_set])
                                    
    
 
